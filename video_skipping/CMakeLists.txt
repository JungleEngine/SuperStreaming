cmake_minimum_required(VERSION 3.9)
project(video_skipping)

set(CMAKE_CXX_STANDARD 14)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
find_path(AVCODEC_INCLUDE_DIR libavcodec/avcodec.h)
find_library(AVCODEC_LIBRARY avcodec)
find_path(AVFORMAT_INCLUDE_DIR libavformat/avformat.h)
find_library(AVFORMAT_LIBRARY avformat)
find_path(AVUTIL_INCLUDE_DIR libavutil/avutil.h libavutil/timestamp.h libavutil/opt.h)
find_library(AVUTIL_LIBRARY avutil)

find_path(AVRESAMPLE_INCLUDE_DIR libavresample/avresample.h)
find_library(AVRESAMPLE_LIBRARY avresample)

find_path(LIBSWSCALE_INCLUDE_DIRS libswscale/swscale.h)
find_library(SWSCALE_LIBRARY swscale)

add_executable(video_skipping src/main.cpp src/VideoContext.h src/VideoContext.cpp src/VideoFrame.h src/VideoFrame.cpp src/queue.h)
target_include_directories(video_skipping PRIVATE ${AVCODEC_INCLUDE_DIR} ${AVFORMAT_INCLUDE_DIR} ${AVUTIL_INCLUDE_DIR}
        ${LIBSWSCALE_INCLUDE_DIRS})
target_link_libraries(video_skipping PRIVATE ${AVCODEC_LIBRARY} ${AVFORMAT_LIBRARY} ${AVUTIL_LIBRARY}
        ${SWSCALE_LIBRARY} Threads::Threads)
