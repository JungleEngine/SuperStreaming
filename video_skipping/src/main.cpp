#include "VideoContext.h"

int main(int argc, char **argv)
{

    std::string input_filename = "/media/syrix/programms/projects/GP/SuperStreaming/ffmpeg_examples/video_skipping/baz.mkv";
    std::string output_filename = "/media/syrix/programms/projects/GP/SuperStreaming/ffmpeg_examples/video_skipping/baz2.mkv";
    VideoContext* video_cntx = new VideoContext(input_filename, output_filename);


    video_cntx->runSkipping();

}