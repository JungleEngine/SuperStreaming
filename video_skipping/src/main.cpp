#include "VideoContext.h"

int main(int argc, char **argv)
{

    std::string input_filename = "/media/syrix/programms/projects/GP/SuperStreaming/video_skipping/baz.mkv";
    std::string output_filename = "/media/syrix/programms/projects/GP/SuperStreaming/video_skipping/baz_skipped.mkv";
    VideoContext* video_cntx = new VideoContext(input_filename, output_filename);



    video_cntx->runSkipping(true);
    video_cntx->close_input_file();
    video_cntx->close_output_file();

}