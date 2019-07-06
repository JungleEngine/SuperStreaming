//
// Created by syrix on 6/27/19.
//

#ifndef DROP_FRAMES_VIDEOFRAME_H
#define DROP_FRAMES_VIDEOFRAME_H
#include <iostream>
#include<string>
#include <sstream>
#include <iterator>
#include <vector>

class VideoFrame {
public:
    int coded_picture_number;
    int64_t pts;
    int64_t duration;
    int repeat_pict;

    VideoFrame(int coded_pict_number, int64_t pts, int64_t duration, int repeat_pict);
    void loadString(std::string);
    std::string toString();
};


#endif //DROP_FRAMES_VIDEOFRAME_H
