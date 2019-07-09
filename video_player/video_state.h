//
// Created by syrix on 7/8/19.
//

#ifndef TRANSCODING_VIDEO_STATE_H
#define TRANSCODING_VIDEO_STATE_H

#include <unistd.h>
#include <stdio.h>
#include <assert.h>
#include <math.h>
#include <time.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_thread.h>
#include<fstream>
#include "VideoFrame.h"

extern "C" {
#include <libavutil/avstring.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/time.h>
#include <libavutil/timestamp.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>

}

#include<iostream>
#include<algorithm>
#include<vector>
#include<string>

using namespace std;

/**
 * Debug flag.
 */
#define _DEBUG_ 0

/**
 * SDL audio buffer size in samples.
 */
#define SDL_AUDIO_BUFFER_SIZE 1024

/**
 * Maximum number of samples per channel in an audio frame.
 */
#define MAX_AUDIO_FRAME_SIZE 192000

/**
 * Audio packets queue maximum size.
 */

#define MAX_AUDIOQ_SIZE (5 * 16 * 1024)
#define MAX_VIDEOQ_SIZE (5 * 256 * 1024)

/**
 * Video packets queue maximum size.
 */
#define MAX_VIDEOQ_SIZE (26549449 * 10)


/**
 * AV sync correction is done if the clock difference is above the maximum AV sync threshold.
 */
#define AV_SYNC_THRESHOLD 0.01

/**
 * No AV sync correction is done if the clock difference is below the minimum AV sync threshold.
 */
#define AV_NOSYNC_THRESHOLD 10.0

/**
 *
 */
#define SAMPLE_CORRECTION_PERCENT_MAX 10

/**
 *
 */
#define AUDIO_DIFF_AVG_NB 20

/**
 * Custom SDL_Event type.
 * Notifies the next video frame has to be displayed.
 */
#define FF_REFRESH_EVENT (SDL_USEREVENT)

/**
 * Custom SDL_Event type.
 * Notifies the program needs to quit.
 */
#define FF_QUIT_EVENT (SDL_USEREVENT + 1)

/**
 * Video Frame queue size.
 */
#define VIDEO_PICTURE_QUEUE_SIZE 3

#define DEFAULT_AV_SYNC_TYPE AV_SYNC_AUDIO_MASTER

/**
 * Prevents SDL from overriding main().
 */


/**
 * Queue structure used to store AVPackets.
 */
class PacketQueue {
public:
    AVPacketList *first_pkt;
    AVPacketList *last_pkt;
    int nb_packets;
    int size;
    SDL_mutex *mutex;
    SDL_cond *cond;
};

/**
 * Queue structure used to store processed video frames.
 */
class VideoPicture {
public:
    AVFrame *frame;
    int width;
    int height;
    int allocated;
    double pts;
};

SDL_Window *screen = NULL;
SDL_Renderer *Renderer;


/**
 * Struct used to hold the format context, the indices of the audio and video stream,
 * the corresponding AVStream objects, the audio and video codec information,
 * the audio and video queues and buffers, the global quit flag and the filename of
 * the movie.
 */



/**
 * Class used to hold data fields used for audio resampling.
 */
class AudioResamplingState {
public:
    SwrContext *swr_ctx;
    int64_t in_channel_layout;
    uint64_t out_channel_layout;
    int out_nb_channels;
    int out_linesize;
    int in_nb_samples;
    int64_t out_nb_samples;
    int64_t max_out_nb_samples;
    uint8_t **resampled_data;
    int resampled_data_size;

};

/**
 * Audio Video Sync Types.
 */
enum {
    /**
     * Sync to audio clock.
     */
            AV_SYNC_AUDIO_MASTER,

    /**
     * Sync to video clock.
     */
            AV_SYNC_VIDEO_MASTER,

    /**
     * Sync to external clock: the computer clock
     */
            AV_SYNC_EXTERNAL_MASTER,
};

class VideoState {
public:
    bool outOFTime= false;
    int finished_decoding = 0;
    bool skippingMode = false;
    /**
     * File I/O Context.
     */
    AVFormatContext *pFormatCtx;
    std::vector<VideoFrame> skipped_frames;

    /**
     * Audio Stream.
     */
    int audioStream;
    AVStream *audio_st;
    AVCodecContext *audio_ctx;
    PacketQueue audioq;
    uint8_t audio_buf[(MAX_AUDIO_FRAME_SIZE * 3) / 2];
    unsigned int audio_buf_size;
    unsigned int audio_buf_index;
    AVFrame audio_frame;
    AVPacket audio_pkt;
    uint8_t *audio_pkt_data;
    int audio_pkt_size;
    double audio_clock;
    int audio_hw_buf_size;

    /**
     * Video Stream.
     */
    int videoStream;
    AVStream *video_st;
    AVCodecContext *video_ctx;
    SDL_Texture *texture;
    SDL_Renderer *renderer;
    PacketQueue videoq;
    SwsContext *sws_ctx;
    double frame_timer;
    double frame_last_pts;
    double frame_last_delay;
    double video_clock;
    double video_current_pts;
    int64_t video_current_pts_time;
    double audio_diff_cum;
    double audio_diff_avg_coef;
    double audio_diff_threshold;
    int audio_diff_avg_count;

    VideoPicture pictq[VIDEO_PICTURE_QUEUE_SIZE];
    int pictq_size;
    int pictq_rindex;
    int pictq_windex;
    SDL_mutex *pictq_mutex;
    SDL_cond *pictq_cond;

    SDL_Renderer *Renderer;
    /**
     * Threads.
     */
    SDL_Thread *decode_tid;
    SDL_Thread *video_tid;

    /**
     * AV Sync.
     */
    int av_sync_type;
    double external_clock;
    int64_t external_clock_time;

    /**
     * Input file name.
     */
    string filename;
    string index_filename;
    ifstream* indexIFStream;
    AVRational indexFileTimebase;


    /**
     * Global quit flag.
     */
    int quit;

    /**
     * Maximum number of frames to be decoded.
     */
    int currentFrameIndex;
    int window_width;
    int window_height;

    ~VideoState(){
        this->indexIFStream->close();
        delete this->indexIFStream;
        this->indexIFStream = nullptr;
        avcodec_free_context(&video_ctx);
        sws_freeContext(sws_ctx);
        SDL_free(screen);
        SDL_free(texture);
    }
};


#endif //TRANSCODING_VIDEO_STATE_H

