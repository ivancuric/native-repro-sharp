cmake_minimum_required(VERSION 3.15)

project (video-module)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(WIN32)
set(FFMPEG_DIR ${CMAKE_SOURCE_DIR}/FFmpeg/win)
else()
set(FFMPEG_DIR ${CMAKE_SOURCE_DIR}/FFmpeg/macos)
endif()

message(STATUS "Assuming FFmpeg build is located in " ${FFMPEG_DIR})

set(UTILS_SRC
  src/utils/PerfLogger.cpp
  src/utils/SharedMemory.cpp
)

if(WIN32)
set(UTILS_SRC ${UTILS_SRC} src/utils/SharedMemoryWin.cpp)
else()
set(UTILS_SRC ${UTILS_SRC} src/utils/SharedMemoryMac.cpp)
endif()

set(NODE_SRC
  src/node/DummyStream.cpp
  src/node/FFmpegStream.cpp
  src/node/Frame.cpp
  src/node/PerfLoggerWrapper.cpp
  src/node/RemoteStream.cpp
  src/node/Stream.cpp
  src/node/Utils.cpp
  src/node/Video.cpp
  src/node/VideoMode.cpp
)

macro(ffmpeg_lib)
add_library(${ARGV0} SHARED IMPORTED)
if(WIN32)
set_target_properties(${ARGV0} PROPERTIES IMPORTED_IMPLIB ${FFMPEG_DIR}/lib/${ARGV0}.lib)
endif()
set_target_properties(${ARGV0} PROPERTIES IMPORTED_LOCATION ${FFMPEG_DIR}/lib/lib${ARGV0}.dylib)
endmacro()

ffmpeg_lib(avcodec)
ffmpeg_lib(avdevice)
ffmpeg_lib(avformat)
ffmpeg_lib(avutil)
ffmpeg_lib(swscale)
if(WIN32)
ffmpeg_lib(avfilter)
ffmpeg_lib(swresample)
ffmpeg_lib(libaacs)
ffmpeg_lib(libbdplus)
ffmpeg_lib(libopenh264)
ffmpeg_lib(postproc)
endif()

if(WIN32)
set(CMAKE_CXX_FLAGS "-DNAPI_CPP_EXCEPTIONS /EHsc")
else()
set(CMAKE_CXX_FLAGS "-Weverything -std=c++17 -Wno-c++98-compat -Wno-padded -Wno-format-nonliteral")
endif()

set(FFMPEG_SRC
  src/ffmpeg/ffmpeg.cpp
  src/ffmpeg/StreamContext.cpp
  src/ffmpeg/VideoMode.cpp
  src/ffmpeg/AVFrameData.cpp
  src/ffmpeg/CapturePrint.cpp
)

set(LIB_SRC
  ${FFMPEG_SRC}
  ${NODE_SRC}
  ${UTILS_SRC}
)

add_library(${PROJECT_NAME} SHARED ${LIB_SRC} ${CMAKE_JS_SRC})

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_JS_INC})
target_include_directories(${PROJECT_NAME} PRIVATE ${FFMPEG_DIR}/include)

set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "" SUFFIX ".node")
target_link_libraries(${PROJECT_NAME} ${CMAKE_JS_LIB})
target_link_libraries(${PROJECT_NAME} avcodec)
target_link_libraries(${PROJECT_NAME} avdevice)
target_link_libraries(${PROJECT_NAME} avformat)
target_link_libraries(${PROJECT_NAME} avutil)
target_link_libraries(${PROJECT_NAME} swscale)

if(WIN32)
target_link_libraries(${PROJECT_NAME} swresample)
target_link_libraries(${PROJECT_NAME} avfilter)
target_link_libraries(${PROJECT_NAME} postproc)
endif()

# Include N-API wrappers
execute_process(COMMAND node -p "require('node-addon-api').include"
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  OUTPUT_VARIABLE NODE_ADDON_API_DIR
)
string(REPLACE "\n" "" NODE_ADDON_API_DIR ${NODE_ADDON_API_DIR})
string(REPLACE "\"" "" NODE_ADDON_API_DIR ${NODE_ADDON_API_DIR})

target_include_directories(${PROJECT_NAME} PRIVATE ${NODE_ADDON_API_DIR})

add_definitions(-DNAPI_VERSION=6)

if(WIN32)
# Copy FFmpeg into build dir
add_custom_command(
  TARGET ${PROJECT_NAME} POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
  ${FFMPEG_DIR}/lib
  $<TARGET_FILE_DIR:${PROJECT_NAME}>
)
else()
# Copy lib files + mangle rpaths
add_custom_command(
  TARGET ${PROJECT_NAME} POST_BUILD
  COMMAND ${CMAKE_SOURCE_DIR}/scripts/macos-mangle-libs.sh ${FFMPEG_DIR}/lib $<TARGET_FILE_DIR:${PROJECT_NAME}>
)
endif()
