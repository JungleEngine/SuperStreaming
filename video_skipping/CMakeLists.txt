cmake_minimum_required(VERSION 3.9)
project(video_skipping)

set(CMAKE_CXX_STANDARD 14)


set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)


add_executable(video_skipping src/main.cpp src/VideoContext.h src/VideoContext.cpp src/VideoFrame.h src/VideoFrame.cpp src/queue.h
        )

target_link_libraries(video_skipping PRIVATE -lavcodec -lavformat -lavutil  -lswscale Threads::Threads)
