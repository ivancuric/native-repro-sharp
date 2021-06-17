#include "SharedMemoryWin.hpp"

namespace utils {

  SharedMemoryWin::SharedMemoryWin()
  {
    m_writer = true;
  }

  SharedMemoryWin::SharedMemoryWin(const std::string& name)
    : m_memoryId(name)
  {
    m_writer = false;
  }

  SharedMemoryWin::~SharedMemoryWin()
  {
  }

  std::optional<std::string> SharedMemoryWin::write(std::vector<std::shared_ptr<video::FrameData>> data) {
    return std::make_optional(std::string("Not yet implemented for Windows"));
  }

  std::vector<std::shared_ptr<video::FrameData>> SharedMemoryWin::read() {
    return std::vector<std::shared_ptr<video::FrameData>>();
  }

  const std::string& SharedMemoryWin::memoryId() const {
    return m_memoryId;
  }
}
