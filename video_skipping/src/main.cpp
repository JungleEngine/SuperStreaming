#include "VideoContext.h"

int main(int argc, char **argv)
{

<<<<<<< HEAD
    std::string input_filename = "/media/syrix/programms/projects/GP/SuperStreaming/video_skipping/animation.mkv";
    std::string output_filename = "/media/syrix/programms/projects/GP/SuperStreaming/video_skipping/animation_skipped.mkv";

=======
    std::string input_filename = "youtube.mp4";
    std::string output_filename = "out.mp4";
>>>>>>> 4a2b00c3aef7a025deb2e612a8f698ca4a667eaa
    VideoContext* video_cntx = new VideoContext(input_filename, output_filename);



    video_cntx->runSkipping(true);
    video_cntx->close_input_file();
    video_cntx->close_output_file();

}