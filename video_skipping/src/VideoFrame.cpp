//
// Created by syrix on 6/27/19.
//

#include "VideoFrame.h"

VideoFrame::VideoFrame(int coded_pict_number, int64_t pts, int64_t duration, int repeat_pict) {
    this->coded_picture_number = coded_pict_number;
    this->pts = pts;
    this->duration = duration;
    this->repeat_pict = repeat_pict;

}



void VideoFrame::loadString(std::string line) {
    std::istringstream buf(line);
    std::istream_iterator<std::string> beg(buf), end;

    std::vector<std::string> tokens(beg, end); // done!

    this->coded_picture_number = std::stoi(tokens[1]);
    this->pts = std::stoi(tokens[3]);
    this->duration = std::stoi(tokens[5]);
    this->repeat_pict = std::stoi(tokens[7]);
}

std::string VideoFrame::toString() {
    std::string str = "cpn: " +
            std::to_string(this->coded_picture_number) + " " +
            "pts: " +  std::to_string(this->pts) + " " +
            "duration: " +  std::to_string(this->duration) + " " +
            "repeat_pict: " +  std::to_string(this->repeat_pict);
    return str;
}
