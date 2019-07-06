#include "VideoContext.h"

int main(int argc, char **argv)
{

    std::string input_filename = "youtube.mp4";
    std::string output_filename = "out.mp4";
    VideoContext* video_cntx = new VideoContext(input_filename, output_filename);



    video_cntx->runSkipping(true);
    video_cntx->close_input_file();
    video_cntx->close_output_file();

}