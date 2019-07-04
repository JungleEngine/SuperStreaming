cmake_minimum_required(VERSION 3.9)
project(transcoding)
set(CMAKE_CXX_FLAGS "-g -O0 -Wall -fprofile-arcs -ftest-coverage")
set(CMAKE_CXX_STANDARD 11)
find_path(AVCODEC_INCLUDE_DIR libavcodec/avcodec.h)
find_library(AVCODEC_LIBRARY avcodec)
find_path(AVFORMAT_INCLUDE_DIR libavformat/avformat.h)
find_library(AVFORMAT_LIBRARY avformat)
find_path(AVUTIL_INCLUDE_DIR libavutil/avutil.h libavutil/timestamp.h libavutil/opt.h)
find_library(AVUTIL_LIBRARY avutil)


find_path(AVRESAMPLE_INCLUDE_DIR libavresample/avresample.h)
find_library(AVRESAMPLE_LIBRARY avresample)

#find_library(SDL_IMAGE_LIBRARY avresample)

find_path(LIBSWSCALE_INCLUDE_DIRS libswscale/swscale.h)
find_library(SWSCALE_LIBRARY swscale)

find_path(SWRESAMPLE_INCLUDE_DIR libswresample/swresample.h)
find_library(SWRESAMPLE_LIBRARY swresample)

find_file(SDL2_INCLUDE_DIR SDL2/SDL.h SDL2/SDL_thread.h)
find_library(SDL2_LIBRARY SDL2)

link_libraries(gcov)

add_executable(transcoding main.cpp player.h)
target_include_directories(transcoding PRIVATE ${AVCODEC_INCLUDE_DIR} ${AVFORMAT_INCLUDE_DIR} ${AVUTIL_INCLUDE_DIR}
	${AVRESAMPLE_INCLUDE_DIR} ${LIBSWSCALE_INCLUDE_DIRS} ${SDL2_INCLUDE_DIR} ${SWRESAMPLE_INCLUDE_DIR})
target_link_libraries(transcoding PRIVATE ${AVCODEC_LIBRARY} ${AVFORMAT_LIBRARY} ${AVUTIL_LIBRARY} ${AVRESAMPLE_LIBRARY}
	${SWSCALE_LIBRARY} ${SDL2_LIBRARY} ${SWRESAMPLE_LIBRARY} )
