#include "SharedMemory.hpp"

#ifndef _WIN32
#include "SharedMemoryMac.hpp"
#else
#include "SharedMemoryWin.hpp"
#endif

namespace utils {

  std::unique_ptr<SharedMemory> SharedMemory::initWriter() {
#ifndef _WIN32
    return std::make_unique<SharedMemoryMac>();
#else
    return std::make_unique<SharedMemoryWin>();
#endif
  }

  std::unique_ptr<SharedMemory> SharedMemory::initReader(const std::string& id) {
#ifndef _WIN32
    return std::make_unique<SharedMemoryMac>(id);
#else
    return std::make_unique<SharedMemoryWin>();
#endif
  }

  SharedMemory::~SharedMemory() {}

}
