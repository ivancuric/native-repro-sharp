#pragma once

#include "../node/Frame.hpp"

#include <memory>
#include <optional>

namespace utils {

  class SharedMemory {
  public:
    typedef std::function<void(std::optional<std::string>)> InitCallback;

    static std::unique_ptr<SharedMemory> initWriter();
    static std::unique_ptr<SharedMemory> initReader(const std::string& id);

    virtual ~SharedMemory();

    // Returns error message in the case of failure
    virtual std::optional<std::string> write(std::vector<std::shared_ptr<video::FrameData>> data) = 0;
    virtual std::vector<std::shared_ptr<video::FrameData>> read() = 0;

    virtual const std::string& memoryId() const = 0;
  };
}
