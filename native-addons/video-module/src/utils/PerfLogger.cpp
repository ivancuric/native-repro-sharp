#include "PerfLogger.hpp"

#include <fstream>
#include <functional>
#include <map>

namespace utils {

  static std::map<std::string, std::shared_ptr<PerfLogger>> s_instances;

  std::ostream& operator<<(std::ostream& os, Key k) {
    switch(k) {
      case Key::Received : os << "received"; break;
      case Key::Decoded  : os << "decoded";  break;
      case Key::Produced : os << "produced"; break;
      case Key::Handling : os << "handling"; break;
      case Key::Rendered : os << "rendered"; break;
    }
    return os;
  }

  std::shared_ptr<PerfLogger> PerfLogger::instance(const std::string& name) {
    std::shared_ptr<PerfLogger>& logger = s_instances[name];
    if(!logger) {
      logger = std::make_shared<PerfLogger>(name);
    }
    return logger;
  }

  std::shared_ptr<PerfLogger> PerfLogger::instance(const std::string& name, bool writeToFile) {
    std::shared_ptr<PerfLogger> logger = instance(name);
    logger->setWriteToFile(writeToFile);
    return logger;
  }

  void PerfLogger::logEntry(const std::string& name, Key key, int n, bool writeToFile) {
    auto logger = instance(name);
    logger->setWriteToFile(writeToFile);
    logger->log(key, n);
  }

  PerfLogger::PerfLogger(const std::string& name)
    : m_name(name),
      m_flushThread(nullptr),
      m_writeToFile(false),
      m_fileInitialized(false)
  {
  }

  PerfLogger::~PerfLogger() {
    if (m_writeToFile) {
      releaseMemory(true);
    }
  }

  void PerfLogger::log(Key key, int n) {
    auto now = std::chrono::high_resolution_clock::now();
    auto epoch = now.time_since_epoch();
    auto d = std::chrono::duration_cast<DurationType>(epoch);
    size_t elements = 0;
    {
      std::lock_guard<std::mutex> g(m_logMutex);
      m_logs.push_back(std::make_tuple(key, n, d));
      elements = m_logs.size();
    }
    if (elements > 1000) {
      releaseMemory(false);
    }
  }

  void PerfLogger::setWriteToFile(bool write) {
    m_writeToFile = write;
    if (!write) {
      m_fileInitialized = false;
    }
  }

  std::string PerfLogger::filename() const {
    return "perf_" + m_name + ".log";
  }

  std::pair<int, std::map<Key, PerfLogger::DurationType>> PerfLogger::latestFrameStats() {
    std::map<int, std::map<Key, DurationType>> candidates;
    {
      std::lock_guard<std::mutex> g(m_logMutex);
      for (auto it = m_logs.rbegin(); it != m_logs.rend(); ++it) {
        auto t = *it;
        const int frame = std::get<int>(t);
        auto& map = candidates[frame];
        map[std::get<Key>(t)] = std::get<DurationType>(t);
        if (map.size() == static_cast<size_t>(Key::Rendered) + 1) {
          return std::make_pair(frame, map);
        }
      }
      return std::make_pair(0, std::map<Key, DurationType>());
    }
  }

  void PerfLogger::releaseMemory(bool lastRelease) {
    decltype(m_logs) copy;
    std::function<void(void)> f;
    {
      std::lock_guard<std::mutex> tg(m_threadMutex);
      if (m_flushThread) {
        m_flushThread->join();
        m_flushThread.reset();
      }
      {
        std::lock_guard<std::mutex> g(m_logMutex);
        std::swap(copy, m_logs);
        if (!lastRelease) {
          size_t toCopy = static_cast<size_t>(std::min(20, std::max(static_cast<int>(copy.size()) - 20, 0)));
          size_t copySize = copy.size() - toCopy;
          m_logs.resize(toCopy);
          std::copy(copy.begin() + static_cast<int>(copySize), copy.end(), m_logs.begin());
          copy.resize(copySize);
        }
      }
      if (copy.empty() || !m_writeToFile) {
        return;
      }
      std::string fname = filename();
      bool initialized = m_fileInitialized;
      m_fileInitialized = true;
      auto flusher = [copy, fname, initialized] {
        std::fstream fs;
        if (!initialized) {
          fs.open(fname, std::fstream::out);
          fs << "n,key,time\n";
        } else {
          fs.open(fname, std::fstream::app | std::fstream::out);
        }
        for(auto t : copy) {
          fs << std::get<int>(t) << ',' << std::get<Key>(t) << ','
             << std::get<DurationType>(t).count() << '\n';
        }
        fs.close();
      };
      if (!lastRelease) {
        m_flushThread = std::make_unique<std::thread>(flusher);
      } else {
        f = std::move(flusher);
      }
    }
    if (lastRelease) {
      f();
    }
  }

} //namespace utils
