//
// Created by syrix on 6/27/19.
//

#ifndef DROP_FRAMES_VIDEOCONTEXT_H
#define DROP_FRAMES_VIDEOCONTEXT_H
#include<iostream>
#include <algorithm>
#include<string>
#include <thread>
#include "queue.h"
extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/timestamp.h>
#include <libavutil/opt.h>
}


class VideoContext {
private:
    std::mutex mutex_;
    std::unique_ptr<PacketQueue> packet_queue_;
    std::unique_ptr<FrameQueue> frame_queue_;
    std::unique_ptr<FrameQueue> processed_frame_queue_;

    std::vector<std::thread> stages_;
    std::exception_ptr exception_{};
    int t =0;

public:
    AVCodecContext *video_dec_cntx;
    AVCodecContext *audio_dec_cntx;

    AVCodecContext *video_enc_cntx;
    AVCodecContext *audio_enc_cntx;

    AVFormatContext *ifmt_ctx;
    AVFormatContext *ofmt_ctx;

    AVOutputFormat *ofmt;

    int video_stream_indx;
    int audio_stream_indx;

    std::string input_filename;
    std::string output_filename;


    VideoContext(std::string& input_filename, std::string& output_filename);

    int openInputFile();
    int openOutputFile();

    int getDecoderCntx();
    int getEncoderCntx();

    void runSkipping();

    void demultiplex();
    void decode();
    void processFrames();
    void encode();


    int writeHeader();
    int writePacket();

//    int decodeFrames();

    void close_input_file();
    void close_output_file();


    bool sendPacketToDecoder(AVPacket *packet);

    bool receiveFrameFromDecoder(AVFrame *frame);

    bool sendFrameToEncoder(AVFrame *frame);

    bool receivePacketFromEncoder(AVPacket *packet);

    int writePacket(AVPacket *);
};


#endif //DROP_FRAMES_VIDEOCONTEXT_H
