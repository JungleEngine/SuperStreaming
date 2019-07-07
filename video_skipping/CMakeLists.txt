cmake_minimum_required(VERSION 3.9)
project(video_skipping)

set(CMAKE_CXX_STANDARD 17)
find_package(Threads REQUIRED)

set(THREADS_PREFER_PTHREAD_FLAG ON)
add_executable(video_skipping src/main.cpp src/VideoContext.h src/VideoContext.cpp src/VideoFrame.h src/VideoFrame.cpp src/queue.h
        src/Skipping.cpp src/Skipping.h  src/libs/cppflow/src/Model.cpp src/libs/cppflow/src/Tensor.cpp)

find_package(OpenCV REQUIRED)


include_directories(${OpenCV_INCLUDE_DIRS}  src/libs/cppflow/include)
target_link_libraries(video_skipping PRIVATE -lavcodec -lavformat -lavutil -lswscale  Threads::Threads
        -ltensorflow ${OpenCV_LIBS})
