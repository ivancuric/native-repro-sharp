#pragma once

#include "napi_include.hpp"

#include <chrono>
#include <unordered_map>
#include <mutex>
#include <string>

#include "Frame.hpp"
#include "../ffmpeg/VideoMode.hpp"
#include "../utils/SharedMemory.hpp"

namespace video {
  // micro seconds from epoch
  typedef std::chrono::duration<long, std::micro> TimeStamp;

  struct EventData {
    EventData(const std::string& type);
    /* Possible payloads */
    std::string type;
    TimeStamp ts;
  };

  class Stream;
  typedef int StreamCacheKey;

  void callFrameCB(
    Napi::Env env,
    Napi::Function callback,
    Stream* ctx,
    StreamCacheKey* data);

  using ThreadSafeFrameCB =
    Napi::TypedThreadSafeFunction<Stream, StreamCacheKey, callFrameCB>;

  void callInitCB(
    Napi::Env env,
    Napi::Function,
    Stream* ctx,
    std::optional<std::string>* error);

  using SharedMemoryInitCB =
    Napi::TypedThreadSafeFunction<Stream, std::optional<std::string>, callInitCB>;

  void callEventCB(Napi::Env env, Napi::Function function, std::nullptr_t*, EventData*);

  using EventCallback = Napi::TypedThreadSafeFunction<std::nullptr_t, EventData, callEventCB>;

  // Common functionality of streams, the actual implementations are
  // in the subclasses
  class Stream {
  public:
    typedef StreamCacheKey CacheKey;
    Stream();
    Stream(const std::string& name);
    ~Stream();

    void stop(bool removeEventListenerFunc=true);

    void setName(const std::string& name);
    Napi::Value name(const Napi::CallbackInfo& info);

    const std::string& cppName() const;
    ffmpeg::VideoMode videoMode(const Napi::CallbackInfo& info) const;

    // Enables remote stream.
    // Returns promise of a string that needs to be used for opening RemoteStream.
    // In case of the failure promise is rejected
    Napi::Value enableRemoteStream(const Napi::CallbackInfo& info);
    void disableRemoteStream(const Napi::CallbackInfo& info);

    // If there is need to have more sophisticated control this could
    // return id of callback or similar
    void addFrameCallback(const Napi::CallbackInfo& info);
    void clearFrameCallbacks(const Napi::CallbackInfo&);
    void clearFrameCallbacks();

    void frameProduced(std::vector<std::shared_ptr<FrameData>> data, bool profile);

    CacheKey* addToCache(std::vector<std::shared_ptr<FrameData>> data);
    void checkCacheRefs(CacheKey key); // removes if refs == 0
    void incrementCacheRefs(CacheKey key);
    std::vector<std::shared_ptr<FrameData>> consumeCacheRef(CacheKey key);

    void sharedMemoryInit(Napi::Env env, std::optional<std::string>& error);

    Napi::Value latestFrameStats(const Napi::CallbackInfo& info);

    void setEventListener(const Napi::CallbackInfo& info);
    void removeEventListener();

    // Events
    void emitStreamStarted();
    void emitStreamStopped();
    void emitStreamStartFailed();
    void emitStreamStopRequested();
    void emitStreamFatalError();
    void emitStreamStartedRecording();
    void emitStreamStoppedRecording();
    void emitStreamSnapShotTaken();
    void emitStreamFailedRecording(const std::string& error);
    void emitEvent(EventData* event);

  private:
    std::vector<ThreadSafeFrameCB> m_frameCallbacks;
    std::string m_name;

    std::mutex m_cacheMutex;

    /**
     *  Getting FrameData out of the arbitrary worker thread for the Frame-
     *  object tied to JS runtime requires some technical plumbing.
     *
     *  We want to have FrameData managed by shared_ptr which frees the
     *  associated FrameData-object when the last reference is removed.
     *  This enables both proper copying of JS-objects and full control on
     *  C++-side if the need arises.
     *
     *  Based on the above requirements we need to ensure that the C++-side
     *  of Stream's API takes in shared_ptr. This is now implemented in
     *  frameProduced-function.
     *
     *  Because frameProduced can be called from arbitrary thread the callbacks
     *  executed in Node runtime's main thread are not immediately executed. The
     *  passed FrameData needs to be kept alive at least as long before the
     *  callbacks have been executed. The only foolproof way to implement this
     *  is to store reference into intermediate container once the FrameData is
     *  passed to the Stream and then fetch the reference at the time of the
     *  callback execution.
     */
    struct CacheItem {
      /// The actual data
      std::vector<std::shared_ptr<FrameData>> data;
      /// Key needs to be stored in the item as well, because the callback
      /// expects pointer of CacheKey and we need to ensure that pointer does
      /// point to valid address
      std::unique_ptr<CacheKey> key;
      /// Number of times we need to be able to fetch the item from cache
      int references;
    };
    std::unordered_map<int, CacheItem> m_dataCache;
    std::unique_ptr<utils::SharedMemory> m_sharedMemory;

    SharedMemoryInitCB m_sharedMemoryInitFunction;
    std::unique_ptr<Napi::Promise::Deferred> m_sharedMemoryInitPromise;

    std::mutex m_eventMutex;
    EventCallback m_eventCallback;
  };

} // namespace video
