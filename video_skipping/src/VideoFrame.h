//
// Created by syrix on 6/27/19.
//

#ifndef DROP_FRAMES_VIDEOFRAME_H
#define DROP_FRAMES_VIDEOFRAME_H
#include<iostream>
#include<string>

class VideoFrame {
public:
    int pts;
    int dts;
    int duration;
    int repeat_pict;

    VideoFrame();
//    load_properties(std::string);

};


#endif //DROP_FRAMES_VIDEOFRAME_H
