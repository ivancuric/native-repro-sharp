#pragma once

#include "SharedMemory.hpp"

#include <optional>

namespace utils {

  class SharedMemoryWin : public SharedMemory {
  public:
    SharedMemoryWin(); // <- Ctor for writer
    SharedMemoryWin(const std::string& memoryId); // <- Ctor for reader
    virtual ~SharedMemoryWin() override;

    virtual std::optional<std::string> write(std::vector<std::shared_ptr<video::FrameData>> data) override;
    virtual std::vector<std::shared_ptr<video::FrameData>> read() override;

    virtual const std::string& memoryId() const override;

  private:
    std::string m_memoryId;
    bool m_writer;
  };

}
