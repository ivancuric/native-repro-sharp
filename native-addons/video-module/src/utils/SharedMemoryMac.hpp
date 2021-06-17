#pragma once

#include "SharedMemory.hpp"

#include <optional>
#include <sys/sem.h>
#include <sys/shm.h>

namespace utils {

  class SharedMemoryMac : public SharedMemory {
  public:
    SharedMemoryMac(); // <- Ctor for writer
    SharedMemoryMac(const std::string& memoryId); // <- Ctor for reader
    virtual ~SharedMemoryMac() override;

    virtual std::optional<std::string> write(std::vector<std::shared_ptr<video::FrameData>> data) override;
    virtual std::vector<std::shared_ptr<video::FrameData>> read() override;

    virtual const std::string& memoryId() const override;

  private:
    std::optional<std::string> ensureSegments(size_t segments);
    void releaseSegments();
    size_t writeToSharedMemory(size_t offset, void* data, size_t dataSize);
    size_t readFromSharedMemory(size_t offset, void* data, size_t dataSize);

    void writeControlData(unsigned frame, unsigned dataSize);
    void readControlData(unsigned& frame, unsigned& dataSize);

    void allocateSharedHelpers();
    void freeSharedHelpers();

    // Semaphore helpers
    void acquireControlSemaphore();
    void releaseControlSemaphore();

    void acquireDataSemaphore();
    void releaseDataSemaphore();

    std::string m_memoryId;
    size_t m_segmentSize;
    int m_globalId;

    std::vector<int> m_segmentIds;
    std::vector<void*> m_memorySegments;

    int m_semaphoreId;
    // The following segment contains: frame number and the size of data
    void* m_sharedMemoryControlSegment;
    int m_sharedMemoryControlId;

    bool m_writer;
  };

}
