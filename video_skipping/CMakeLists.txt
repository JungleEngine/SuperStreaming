cmake_minimum_required(VERSION 3.9)
project(video_skipping)

set(CMAKE_CXX_STANDARD 17)
find_package(Threads REQUIRED)

set(THREADS_PREFER_PTHREAD_FLAG ON)
<<<<<<< HEAD
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
=======
add_executable(video_skipping src/main.cpp src/VideoContext.h src/VideoContext.cpp src/VideoFrame.h src/VideoFrame.cpp src/queue.h
        src/Skipping.cpp src/Skipping.h  src/libs/cppflow/src/Model.cpp src/libs/cppflow/src/Tensor.cpp)

find_package(OpenCV REQUIRED)


include_directories(${OpenCV_INCLUDE_DIRS}  src/libs/cppflow/include )
target_link_libraries(video_skipping PRIVATE -lavcodec -lavformat -lavutil -lswscale  Threads::Threads
        -ltensorflow ${OpenCV_LIBS})
<<<<<<< HEAD

=======
>>>>>>> 1ff60c3c04ed058149b559202568e28b41d05632
>>>>>>> 4a2b00c3aef7a025deb2e612a8f698ca4a667eaa
