#pragma once

#include <chrono>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <tuple>
#include <vector>

namespace utils {

  enum class Key {
    Received = 0,
    Decoded = 1,
    Produced = 2,
    Handling = 3,
    Rendered = 4
  };

  std::ostream& operator<<(std::ostream& os, Key k);

  // Functions are thread-safe, but instancing not
  class PerfLogger {
  public:
    typedef std::chrono::duration<long, std::micro> DurationType;

    static std::shared_ptr<PerfLogger> instance(const std::string& name);
    static std::shared_ptr<PerfLogger> instance(const std::string& name, bool writeToFile);
    static void logEntry(const std::string& name, Key key,  int measurement, bool writeToFile=true);

    PerfLogger(const std::string& name);
    ~PerfLogger();

    void log(Key key, int measurement);
    void releaseMemory(bool lastRelease);
    void setWriteToFile(bool write);

    std::pair<int, std::map<Key, DurationType>> latestFrameStats();

  private:
    std::string filename() const;

    std::string m_name;

    std::mutex m_logMutex;
    std::vector<std::tuple<Key, int, DurationType>> m_logs;

    std::mutex m_threadMutex;
    std::unique_ptr<std::thread> m_flushThread;

    bool m_writeToFile;
    bool m_fileInitialized;
  };

} // namespace utils
