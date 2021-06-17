#include "Stream.hpp"

#include "Frame.hpp"
#include "VideoMode.hpp"
#include "../utils/PerfLogger.hpp"

#include <iostream>

using std::chrono::duration_cast;
using std::chrono::high_resolution_clock;

namespace video {

  EventData::EventData(const std::string& _type)
    : type(_type),
      ts(duration_cast<TimeStamp>(high_resolution_clock::now().time_since_epoch()))
  {}

  Stream::Stream() {}

  Stream::Stream(const std::string& name)
    : m_name(name)
  {
  }

  Stream::~Stream() {
    // Destructor. This will eventually be called by GC once all references are
    // removed
    std::shared_ptr<utils::PerfLogger> logger = utils::PerfLogger::instance(cppName());
    logger->releaseMemory(true);
    stop();
  }

  void Stream::stop(bool removeEventListenerFunc) {
    // Clean everything
    clearFrameCallbacks();
    if (removeEventListenerFunc)
      removeEventListener();
    std::shared_ptr<utils::PerfLogger> logger = utils::PerfLogger::instance(cppName());
    logger->releaseMemory(false);
  }

  ffmpeg::VideoMode Stream::videoMode(const Napi::CallbackInfo& info) const {
    Napi::Env env = info.Env();
    bool error = info.Length() < 1 || !info[0].IsObject();
    if(!error) {
      ffmpeg::VideoMode mode = VideoMode::convert(info[0].As<Napi::Object>());
      if (mode.isValid()) {
        return mode;
      }
    }
    throw Napi::TypeError::New(env, "Expected first argument to be an instance of VideoMode");
  }

  void Stream::addFrameCallback(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if(info.Length() < 1 || !info[0].IsFunction()) {
      throw Napi::TypeError::New(env, "Expected first argument to be function");
    }
    // see https://github.com/nodejs/node-addon-api/blob/master/doc/typed_threadsafe_function.md for additional info

    using FinalizerDataType = void;

    m_frameCallbacks.emplace_back(ThreadSafeFrameCB::New(
      env,
      info[0].As<Napi::Function>(),
      "Frame consumer callback",
      0, // unlimited queue
      1, // Only one thread will use this initially
      this, // context
      [](Napi::Env, FinalizerDataType*, Stream*) {
      } /*, FinalizerDataType dataToFinalizer */
    ));
  }

  void Stream::clearFrameCallbacks(const Napi::CallbackInfo&) {
    clearFrameCallbacks();
    std::lock_guard<std::mutex> g(m_cacheMutex);
    m_dataCache.clear();
  }

  void Stream::clearFrameCallbacks() {
    for(auto cb : m_frameCallbacks) {
      cb.Release();
    }
    m_frameCallbacks.clear();
    m_sharedMemory.reset();
  }

  void Stream::setName(const std::string& name) {
    m_name = name;
  }

  Napi::Value Stream::name(const Napi::CallbackInfo& info) {
    return Napi::String::New(info.Env(), m_name);
  }

  const std::string& Stream::cppName() const {
    return m_name;
  }

  void Stream::frameProduced(std::vector<std::shared_ptr<FrameData>> data, bool profile) {
    if (m_sharedMemory) {
      auto error = m_sharedMemory->write(data);
      bool hasError = error.has_value();
      if (m_sharedMemoryInitFunction != nullptr) {
        auto copyPtr = new std::optional<std::string>(error);
        m_sharedMemoryInitFunction.BlockingCall(copyPtr);
        m_sharedMemoryInitFunction.Release();
        m_sharedMemoryInitFunction = SharedMemoryInitCB();
      }
      if (hasError) {
        m_sharedMemory.reset();
      }
    }

    if (!m_frameCallbacks.empty()) {
      CacheKey* keyPtr = addToCache(data);
      CacheKey key = *keyPtr;
      for (auto cb: m_frameCallbacks) {
        incrementCacheRefs(key);
        napi_status status = cb.BlockingCall(keyPtr);
        if (status != napi_ok) {
          // error - what to do?
          consumeCacheRef(key);
        }
      }
      checkCacheRefs(key);
    }
    utils::PerfLogger::logEntry(cppName(), utils::Key::Produced,
                                data[0]->frameNumber(), profile);
  }

  Napi::Value Stream::latestFrameStats(const Napi::CallbackInfo& info) {
    std::shared_ptr<utils::PerfLogger> logger = utils::PerfLogger::instance(cppName());

    auto statsPair = logger->latestFrameStats();
    auto& stats = statsPair.second;

    if (stats.empty()) {
      return info.Env().Undefined();
    }
    Napi::Object obj = Napi::Object::New(info.Env());
    obj.Set("decode", (stats[utils::Key::Decoded] - stats[utils::Key::Received]).count());
    obj.Set("passing", (stats[utils::Key::Produced] - stats[utils::Key::Decoded]).count());
    obj.Set("nodeDelay", (stats[utils::Key::Handling] - stats[utils::Key::Produced]).count());
    obj.Set("render", (stats[utils::Key::Rendered] - stats[utils::Key::Handling]).count());
    obj.Set("total", (stats[utils::Key::Rendered] - stats[utils::Key::Received]).count());
    obj.Set("frameNumber", statsPair.first);
    return obj;
  }

  Stream::CacheKey* Stream::addToCache(std::vector<std::shared_ptr<FrameData>> data) {
    CacheItem item{data, std::make_unique<CacheKey>(), 0};

    std::lock_guard<std::mutex> g(m_cacheMutex);

    auto itemKey = static_cast<int>(m_dataCache.size());
    std::unordered_map<int, CacheItem>::iterator it;
    while (true) {
      it = m_dataCache.find(itemKey);
      if (it == m_dataCache.end()) {
        break;
      }
      ++itemKey;
    }
    *item.key = itemKey;
    auto keyAddr = item.key.get();
    m_dataCache.insert(it, {itemKey, std::move(item)});
    return keyAddr;
  }

  void Stream::checkCacheRefs(CacheKey key) {
    CacheItem item; // do memory deallocation after mutex is freed
    {
      std::lock_guard<std::mutex> g(m_cacheMutex);
      auto it = m_dataCache.find(key);
      if (it != m_dataCache.end() && it->second.references == 0) {
        item = std::move(it->second);
        m_dataCache.erase(it);
      }
    }
  }

  void Stream::incrementCacheRefs(CacheKey key) {
    std::lock_guard<std::mutex> g(m_cacheMutex);
    auto it = m_dataCache.find(key);
    if (it != m_dataCache.end()) {
      ++it->second.references;
    }
  }

  std::vector<std::shared_ptr<FrameData>> Stream::consumeCacheRef(CacheKey key) {
    std::vector<std::shared_ptr<FrameData>> data;
    {
      std::lock_guard<std::mutex> g(m_cacheMutex);
      auto it = m_dataCache.find(key);
      if (it != m_dataCache.end()) {
        data = it->second.data;
        if (--it->second.references == 0) {
          m_dataCache.erase(it);
        }
      }
    }
    return data;
  }

  void callFrameCB(
    Napi::Env env,
    Napi::Function callback,
    Stream* ctx,
    Stream::CacheKey* data)
  {
    Stream::CacheKey key = *data;
    std::vector<std::shared_ptr<FrameData>> frameData = ctx->consumeCacheRef(key);
    if (frameData.empty())
      return;
    if (env != nullptr) {
      if (callback != nullptr) {
        Napi::Value frame = Frame::create(env, frameData);
        // If want to give context (=this) it needs to be first parameter
        // as Napi::Value - Find out how to feed Frame to this
        callback.Call({ frame });
      }
    }
  }

  Napi::Value Stream::enableRemoteStream(const Napi::CallbackInfo& info) {
    m_sharedMemory = utils::SharedMemory::initWriter();

    m_sharedMemoryInitPromise = std::make_unique<Napi::Promise::Deferred>(
      Napi::Promise::Deferred::New(info.Env())
    );
    m_sharedMemoryInitFunction = SharedMemoryInitCB::New(
      info.Env(),
      Napi::Function(), // Dummy function, not really going to be used
      "Shared memory init promise callback",
      0,
      1,
      this
    );
    return m_sharedMemoryInitPromise->Promise();
  }

  void Stream::disableRemoteStream(const Napi::CallbackInfo&) {
    m_sharedMemory.reset();
  }

  void callInitCB(
    Napi::Env env,
    Napi::Function,
    Stream* ctx,
    std::optional<std::string>* error)
  {
    ctx->sharedMemoryInit(env, *error);
    delete error;
  }

  void Stream::sharedMemoryInit(Napi::Env env, std::optional<std::string>& error) {
    if (env != nullptr && m_sharedMemoryInitPromise) {
      if (!error.has_value() && m_sharedMemory != nullptr) {
        m_sharedMemoryInitPromise->Resolve(Napi::String::New(env, m_sharedMemory->memoryId()));
      } else if (error.has_value()) {
        m_sharedMemoryInitPromise->Reject(Napi::String::New(env, *error));
      } else {
        m_sharedMemoryInitPromise->Reject(Napi::String::New(env, "Shared memory was destroyed"));
      }
      m_sharedMemoryInitPromise.reset();
    }
  }

  void Stream::setEventListener(const Napi::CallbackInfo& info) {
    removeEventListener(); // Only one at a time

    Napi::Env env = info.Env();
    if(info.Length() < 1 || !info[0].IsFunction()) {
      throw Napi::TypeError::New(env, "Expected first argument to be function");
    }
    std::lock_guard<std::mutex> g(m_eventMutex);
    m_eventCallback = EventCallback::New(
      env,
      info[0].As<Napi::Function>(),
      "Event listener callback",
      0,
      1);
  }

  void Stream::removeEventListener() {
    std::lock_guard<std::mutex> g(m_eventMutex);
    if (m_eventCallback != nullptr) {
      m_eventCallback.Release();
      m_eventCallback = EventCallback();
    }
  }

  void Stream::emitStreamStarted() {
    EventData *event = new EventData("started");
    emitEvent(event);
  }

  void Stream::emitStreamStopped() {
    EventData *event = new EventData("stopped");
    emitEvent(event);
  }

  void Stream::emitStreamStartFailed() {
    EventData *event = new EventData("start-failed");
    emitEvent(event);
  }

  void Stream::emitStreamStopRequested() {
    EventData *event = new EventData("stop-requested");
    emitEvent(event);
  }

  void Stream::emitStreamFatalError() {
    EventData *event = new EventData("fatal-error");
    emitEvent(event);
  }

  void Stream::emitStreamStartedRecording() {
    EventData *event = new EventData("started-recording");
    emitEvent(event);
  }

  void Stream::emitStreamStoppedRecording() {
    EventData *event = new EventData("stopped-recording");
    emitEvent(event);
  }

  void Stream::emitStreamSnapShotTaken() {
    EventData *event = new EventData("snapshot-taken");
    emitEvent(event);
  }

  void Stream::emitStreamFailedRecording(const std::string& error) {
    EventData *event = new EventData("failed-start-recording: " + error); // TODO: implement payload
    emitEvent(event);
  }

  void Stream::emitEvent(EventData* event) {
    std::lock_guard<std::mutex> g(m_eventMutex);
    if (m_eventCallback != nullptr) {
      m_eventCallback.BlockingCall(event);
    }
  }

  void callEventCB(Napi::Env env, Napi::Function function, std::nullptr_t*, EventData* event) {
    if (env != nullptr && function != nullptr) {
      Napi::Object obj = Napi::Object::New(env);
      obj.Set("type", event->type);
      obj.Set("timestamp", event->ts.count());
      function.Call({ obj });
    }
    delete event;
  }

} // namespace video
