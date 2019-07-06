cmake_minimum_required(VERSION 3.9)
project(transcoding)

set(CMAKE_CXX_STANDARD 14)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

find_file(SDL2_INCLUDE_DIR SDL2/SDL.h SDL2/SDL_thread.h)
find_library(SDL2_LIBRARY SDL2)

add_executable(transcoding main.cpp player.h VideoFrame.h VideoFrame.cpp)
target_include_directories(transcoding PRIVATE  ${SDL2_INCLUDE_DIR} )

target_link_libraries(transcoding PRIVATE  -lavcodec -lavformat -lavutil -lswresample -lswscale Threads::Threads
        ${SDL2_LIBRARY})
