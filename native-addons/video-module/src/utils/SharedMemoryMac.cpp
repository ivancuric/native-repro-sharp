#include "SharedMemoryMac.hpp"

#include <array>
#include <cmath>
#include <set>
#include <string>
#include <variant>

#include <errno.h>
#include <sys/sysctl.h>

namespace utils {
  // The following files are used for parameter to ftok. ftok takes also integer
  // where first 8 bits are used. Normal frame data is stored to keys allocated by
  // 1,2,..., 0xfd (253)
  // - 0xff is reserved for semaphores
  // - 0xfe is reserved for memory area used to save current frame number
  static const char* s_sharedMemoryNames[] = {
    "/dev/null",
    "/dev/stdin",
    "/dev/strdour",
    "/dev/stderr",
    "/dev/urandom",
    "/dev/zero",
    "/Applications",
    "/System"
  };

  static std::set<int> s_sharedMemoryWriters;

  struct SharedAddress {
    size_t targetSegment;
    size_t localOffset;
    size_t segmentLeft;
  };

  static SharedAddress translateAddress(size_t globalOffset, size_t segmentSize) {
    SharedAddress addr;
    addr.targetSegment = globalOffset / segmentSize;
    addr.localOffset = globalOffset % segmentSize;
    addr.segmentLeft = segmentSize - addr.localOffset;
    return addr;
  }

  static uint8_t* mapAddress(const SharedAddress& addr, const std::vector<void*> segments) {
    return static_cast<uint8_t*>(segments[addr.targetSegment]) + addr.localOffset;
  }

    static std::string errorCodeToMsg(int errsv) {
    switch(errsv) {
    case EACCES:
      return "EACCESS";
    case EEXIST:
      return "EEXIST";
    case EINVAL:
      return "EINVAL";
    case ENOENT:
      return "ENOENT";
    case ENFILE:
      return "ENFILE";
    case ENOMEM:
      return "ENOMEM";
    case ENOSPC:
      return "ENOSPC";
    default:
      return "UNKNOWN";
    }
  }

  typedef std::variant<std::pair<int, void*>, std::string> ShMemInit;
  static ShMemInit initShMem(const std::string& path, int segment, size_t segmentSize, bool writer) {
    key_t token = ftok(path.c_str(), segment);

    int flags = writer ? IPC_CREAT | IPC_EXCL | 0644 : SHM_R;
    int shmId = shmget(token, segmentSize, flags);
    if (shmId == -1) {
      int errsv = errno;
      std::string error = "Got " + errorCodeToMsg(errsv) + " during shmget "
        + std::to_string(segment) + " for key " + std::to_string(token);
      return std::move(error);
    }

    flags = writer ? 0 : SHM_RDONLY;
    void* mem = shmat(shmId, nullptr, flags);
    if (mem == reinterpret_cast<void*>(-1)) {
      int errsv = errno;
      std::string error = "Got " + errorCodeToMsg(errsv) + " during shmat "
        + std::to_string(segment) + " for shm id " + std::to_string(shmId);
      return std::move(error);
    }
    return std::make_pair(shmId, mem);
  }

  static std::string exec(const char* cmd) {
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
    if (!pipe) {
      return "";
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
      result += buffer.data();
    }
    return result;
  }

  static size_t getMaxShmSegment() {
    std::string res = exec("sysctl -n kern.sysv.shmmax");
    if (res.empty()) {
      // In case of failure use default value
      return 4194304;
    } else {
      return static_cast<size_t>(std::stoi(res));
    }
  }

  static int allocateGlobalId() {
    for (int i = 0; ; ++i) {
      auto it = s_sharedMemoryWriters.find(i);
      if (it == s_sharedMemoryWriters.end()) {
        s_sharedMemoryWriters.insert(it, i);
        return i;
      }
    }
  }

  SharedMemoryMac::SharedMemoryMac()
    : m_globalId(allocateGlobalId())
  {
    m_memoryId = s_sharedMemoryNames[m_globalId];
    m_segmentSize = getMaxShmSegment();
    m_writer = true;

    allocateSharedHelpers();
  }

  SharedMemoryMac::SharedMemoryMac(const std::string& name)
    : m_memoryId(name),
      m_globalId(-1)
  {
    m_segmentSize = getMaxShmSegment();
    m_writer = false;

    allocateSharedHelpers();
  }

  SharedMemoryMac::~SharedMemoryMac()
  {
    releaseSegments();
    freeSharedHelpers();
    if (m_globalId >= 0) {
      s_sharedMemoryWriters.erase(m_globalId);
    }
  }

  void SharedMemoryMac::allocateSharedHelpers() {
    key_t token = ftok(m_memoryId.c_str(), 0xff);
    int semFlags = m_writer ? IPC_CREAT | IPC_EXCL | 0666 : 0666;
    m_semaphoreId = semget(token, 2, semFlags);

    auto v = initShMem(m_memoryId, 0xfe, 2*sizeof(int), m_writer);
    if (std::holds_alternative<std::string>(v)) {
      throw std::runtime_error(std::get<std::string>(v));
    } else {
      auto segment = std::get<std::pair<int, void*>>(v);
      m_sharedMemoryControlId = segment.first;
      m_sharedMemoryControlSegment = segment.second;
    }
  }

  void SharedMemoryMac::freeSharedHelpers() {
    // Detach
    shmdt(m_sharedMemoryControlSegment);
    if (m_writer) {
      semctl(m_semaphoreId, 0, IPC_RMID);
      shmctl(m_sharedMemoryControlId, IPC_RMID, nullptr);
    }
  }

  std::optional<std::string> SharedMemoryMac::write(std::vector<std::shared_ptr<video::FrameData>> data) {
    // We will write following into shared memory:
    // * number of planes - sizeof(int)
    // * For each plane (as many as there are planes)
    //   * Size of plane in bytes - sizeof(int)
    //   * Height of the plane - sizeof(int)
    //   * data of plane - variable size

    int planes = static_cast<int>(data.size());
    size_t bytesNeeded = static_cast<size_t>(planes * 2 + 1) * sizeof(int);
    for(size_t i = 0; i < data.size(); ++i) {
      bytesNeeded += data[i]->length();
    }
    m_segmentSize = std::min(bytesNeeded, m_segmentSize);
    size_t segmentsNeeded = (bytesNeeded - 1) / m_segmentSize + 1;

    acquireControlSemaphore();

    if (m_memorySegments.empty()) {
      auto error = ensureSegments(segmentsNeeded);
      if (error.has_value()) {
        releaseSegments();
        releaseControlSemaphore();
        return error;
      }
    }

    unsigned frame = data[0]->frameNumber();
    unsigned bytes = static_cast<unsigned>(bytesNeeded);
    writeControlData(frame, bytes);

    acquireDataSemaphore();
    releaseControlSemaphore();

    size_t written = 0;
    written += writeToSharedMemory(written, &planes, sizeof(planes));

    for(size_t i = 0; i < data.size(); ++i) {
      // For each plane
      int meta[2];
      meta[0] = static_cast<int>(data[i]->length());
      meta[1] = static_cast<int>(data[i]->height());
      written += writeToSharedMemory(written, meta, sizeof(meta));
      written += writeToSharedMemory(written, data[i]->data(), static_cast<size_t>(meta[0]));
    }
    releaseDataSemaphore();
    return std::nullopt;
  }

  size_t SharedMemoryMac::writeToSharedMemory(size_t offset, void* data, size_t size) {
    size_t toBeWritten = size;
    size_t hasWrote = 0;
    while (toBeWritten > 0) {
      SharedAddress addr = translateAddress(offset + hasWrote, m_segmentSize);
      size_t toWrite = std::min(addr.segmentLeft, toBeWritten);

      uint8_t* target = mapAddress(addr, m_memorySegments);
      uint8_t* src = static_cast<uint8_t*>(data) + hasWrote;
      std::copy(src, src + toWrite, target);

      hasWrote += toWrite;
      toBeWritten -= toWrite;
    }
    return size;
  }

  void SharedMemoryMac::writeControlData(unsigned frame, unsigned bytes) {
    uint8_t* src = reinterpret_cast<uint8_t*>(&frame);
    uint8_t* target = reinterpret_cast<uint8_t*>(m_sharedMemoryControlSegment);
    std::copy(src, src + sizeof(frame), target);
    target += sizeof(frame);
    src = reinterpret_cast<uint8_t*>(&bytes);
    std::copy(src, src + sizeof(bytes), target);
  }

  void SharedMemoryMac::readControlData(unsigned& frame, unsigned& bytes) {
    uint8_t* src = reinterpret_cast<uint8_t*>(m_sharedMemoryControlSegment);
    std::copy(src, src + sizeof(frame), reinterpret_cast<uint8_t*>(&frame));
    src += sizeof(frame);
    std::copy(src, src + sizeof(frame), reinterpret_cast<uint8_t*>(&bytes));
  }

  std::optional<std::string> SharedMemoryMac::ensureSegments(size_t segments) {
    while (m_segmentIds.size() < segments) {
      int segmentId = static_cast<int>(m_segmentIds.size()) + 1;
      ShMemInit v = initShMem(m_memoryId, segmentId, m_segmentSize, m_writer);
      if (std::holds_alternative<std::string>(v)) {
        return std::make_optional(std::get<std::string>(v));
      } else {
        auto segment = std::get<std::pair<int, void*>>(v);
        m_segmentIds.push_back(segment.first);
        m_memorySegments.push_back(segment.second);
      }
    }
    return std::nullopt;
  }

  void SharedMemoryMac::releaseSegments() {
    // Take semaphore if writer
    for(size_t i = 0; i < m_memorySegments.size(); ++i) {
      shmdt(m_memorySegments[i]);
    }

    for(size_t i = 0; i < m_segmentIds.size(); ++i) {
      if (m_writer) {
        shmctl(m_segmentIds[i], IPC_RMID, nullptr);
      }
    }
  }

  // TODO: Skip reading if frame is already read, (maybe via param?)
  std::vector<std::shared_ptr<video::FrameData>> SharedMemoryMac::read() {
    acquireControlSemaphore();

    unsigned frame;
    unsigned bytes;
    readControlData(frame, bytes);
    m_segmentSize = std::min(m_segmentSize, static_cast<size_t>(bytes));

    size_t segments = (bytes - 1)/m_segmentSize + 1;

    ensureSegments(segments);
    if(m_memorySegments.size() < segments) {
      releaseControlSemaphore();
      return std::vector<std::shared_ptr<video::FrameData>>();
    }

    acquireDataSemaphore();
    releaseControlSemaphore();

    int planes;
    size_t read = 0;
    read += readFromSharedMemory(read, &planes, sizeof(planes));
    if (planes == 0) {
      releaseDataSemaphore();
      return std::vector<std::shared_ptr<video::FrameData>>();
    }
    std::vector<std::shared_ptr<video::FrameData>> result;

    for(int i = 0; i < planes; ++i) {
      int meta[2]; // 0 -> length, 1 -> height
      read += readFromSharedMemory(read, meta, sizeof(meta));
      size_t planeSize = static_cast<size_t>(meta[0]);

      uint8_t* data = new uint8_t[planeSize];
      read += readFromSharedMemory(read, data, planeSize);

      size_t lineSize = planeSize / static_cast<size_t>(meta[1]);
      // We are claiming that width == linesize, which is incorrect for many
      // pixelformats. This doesn't matter as the texture dimensions are sent via
      // other channels
      auto plane = std::make_shared<video::FrameData>(data, meta[0], lineSize, meta[1], frame);
      result.push_back(plane);
    }
    releaseDataSemaphore();
    return result;
  }

  size_t SharedMemoryMac::readFromSharedMemory(size_t offset, void* data, size_t size) {
    size_t toBeRead = size;
    size_t hasRead = 0;
    while (toBeRead > 0) {
      SharedAddress addr = translateAddress(offset + hasRead, m_segmentSize);
      size_t toRead = std::min(addr.segmentLeft, toBeRead);

      uint8_t* src = mapAddress(addr, m_memorySegments);
      uint8_t* target = static_cast<uint8_t*>(data) + hasRead;
      std::copy(src, src + toRead, target);

      hasRead += toRead;
      toBeRead -= toRead;
    }
    return size;

  }

  const std::string& SharedMemoryMac::memoryId() const {
    return m_memoryId;
  }

  void SharedMemoryMac::acquireControlSemaphore() {
    struct sembuf sops[2] = {
      {
        .sem_num = 0,
        .sem_op = 0,
        .sem_flg = 0
      },
      {
        .sem_num = 0,
        .sem_op = 1,
        .sem_flg = 0
      }
    };
    semop(m_semaphoreId, sops, 2);
  }

  void SharedMemoryMac::releaseControlSemaphore() {
    struct sembuf sop = {
      .sem_num = 0,
      .sem_op = -1,
      .sem_flg = 0
    };
    semop(m_semaphoreId, &sop, 1);
  }

  void SharedMemoryMac::acquireDataSemaphore() {
    struct sembuf sops[2] = {
      {
        .sem_num = 1,
        .sem_op = 0,
        .sem_flg = 0
      },
      {
        .sem_num = 1,
        .sem_op = 1,
        .sem_flg = 0
      }
    };
    semop(m_semaphoreId, sops, 2);

  }

  void SharedMemoryMac::releaseDataSemaphore() {
    struct sembuf sop = {
      .sem_num = 1,
      .sem_op = -1,
      .sem_flg = 0
    };
    semop(m_semaphoreId, &sop, 1);
  }

}
