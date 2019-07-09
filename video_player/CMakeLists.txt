cmake_minimum_required(VERSION 3.9)
project(transcoding)

set(CMAKE_CXX_STANDARD 17)



#find_path(AVRESAMPLE_INCLUDE_DIR libavresample/avresample.h)
#find_library(AVRESAMPLE_LIBRARY avresample)

#find_library(SDL_IMAGE_LIBRARY avresample)

find_path(LIBSWSCALE_INCLUDE_DIRS libswscale/swscale.h)
find_library(SWSCALE_LIBRARY swscale)

find_path(SWRESAMPLE_INCLUDE_DIR libswresample/swresample.h)
find_library(SWRESAMPLE_LIBRARY swresample)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)


find_file(SDL2_INCLUDE_DIR SDL2/SDL.h SDL2/SDL_thread.h)
find_library(SDL2_LIBRARY SDL2)

add_executable(transcoding main.cpp player.h VideoFrame.h VideoFrame.cpp video_state.h)
target_include_directories(transcoding PRIVATE  ${SDL2_INCLUDE_DIR} )

target_link_libraries(transcoding PRIVATE  -lavcodec -lavformat -lavutil -lswresample -lswscale Threads::Threads
        ${SDL2_LIBRARY})

