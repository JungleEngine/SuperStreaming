#include <algorithm>
#include <SDL2/SDL.h>
#include <SDL2/SDL_thread.h>
#include <assert.h>
#include <zconf.h>


extern "C"{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>

#include <libavutil/timestamp.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
}

/*  
extern "C"{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>

#include <libavutil/timestamp.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
}
*/
#define av_err2str(errnum) av_make_error_string((char*)__builtin_alloca(AV_ERROR_MAX_STRING_SIZE), AV_ERROR_MAX_STRING_SIZE, errnum)
#define av_ts2str(ts) av_ts_make_string((char*)__builtin_alloca(AV_TS_MAX_STRING_SIZE), ts)

int is_paused = 0;
int last_paused = is_paused;

int received_frames = 0;
int written_packets_count = 0;
int largest_packet_size = 0;
int largest_input_packet_size = 0;

int video_stream_indx = 0;
int audio_stream_indx = 0;
int should_skip_frame = 0;
int skipped_frames = 0;
int first_packet_size = 0;
int first_packet_size_read = 0;

static AVFormatContext *ifmt_ctx;
static AVFormatContext *ofmt_ctx;


typedef struct StreamContext {
    AVCodecContext *dec_ctx;
    AVCodecContext *enc_ctx;
} StreamContext;

static StreamContext *stream_ctx;
int put_all = 0;
// SDL stuff.
struct SwsContext *sws_ctx = NULL;
struct SwrContext *swr;
SDL_Window* window = NULL;
SDL_Surface* screen = NULL;
SDL_Event event;
SDL_Renderer *renderer;
SDL_Texture *texture;
Uint8 *yPlane, *uPlane, *vPlane;
size_t yPlaneSz, uvPlaneSz;
int uvPitch;

// SDL audio stuff
SDL_AudioSpec   wanted_spec, spec;
int quit = 0;
// Ffmpeg defaults.
#define SDL_AUDIO_BUFFER_SIZE 1024
#define MAX_AUDIO_FRAME_SIZE 192000
#define OUT_SAMPLE_FMT AV_SAMPLE_FMT_FLT
typedef struct PacketQueue {
    AVPacketList *first_pkt, *last_pkt;
    int nb_packets;
    int size;
    /*
     * This is because SDL is running the audio process as a separate thread.
     * If we don't lock the queue properly, we could really mess up our data.
     */
    SDL_mutex *mutex;
    SDL_cond *cond;
} PacketQueue;

PacketQueue audioq;
void packet_queue_init(PacketQueue *q) {
    memset(q, 0, sizeof(PacketQueue));
    q->mutex = SDL_CreateMutex();
    q->cond = SDL_CreateCond();
}


//BUG : this function in still buggy- some times the progrom doesn't close safely
void do_quit(AVFrame* frame){

//the following lines (and more) to close the stream {stream_close()}
    av_frame_free(&frame);
    swr_free(&swr);

    for (int i = 0; i < ifmt_ctx->nb_streams; i++) {
        avcodec_free_context(&stream_ctx[i].dec_ctx);
        if (ofmt_ctx && ofmt_ctx->nb_streams > i && ofmt_ctx->streams[i] && stream_ctx[i].enc_ctx)
            avcodec_free_context(&stream_ctx[i].enc_ctx);
    }    
    avformat_close_input(&ifmt_ctx);
    SDL_DestroyTexture(texture);
    av_free(stream_ctx);

    //--------------------------------------------------------
    //you may need to check first if there is render object!
    SDL_DestroyRenderer(renderer);
    //you may need to check first if there is a window object !
    SDL_DestroyWindow(window);

#if CONFIG_AVFILTER
    av_freep(&vfilters_list);//check the usage of this line and differents between ( av_Free() & av_Freep() )
#endif

/*
 *  Undo the initialization done by avformat_network_init. Call it only
 *  once for each time you called avformat_network_init.
 */
    avformat_network_deinit();//check the usage of this line
    SDL_Quit();
    av_log(NULL, AV_LOG_QUIET, "%s", ""); //we may change this line
    exit(0);
}


int packet_queue_put(PacketQueue *q, AVPacket *pkt) {
    AVPacketList *pkt1;
    // init packet
    if(av_packet_ref(pkt, pkt) <0){
        return -1;
    }
    pkt1 = static_cast<AVPacketList *>(av_malloc(sizeof(AVPacketList)));
    if(!pkt1) {
        return -1;
    }
    pkt1->pkt = *pkt;
    pkt1->next = NULL;

    SDL_LockMutex(q->mutex);

    if(!q->last_pkt)
        q->first_pkt = pkt1;
    else
        q->last_pkt->next = pkt1;
    q->last_pkt = pkt1;
    q->nb_packets++;
    q->size += pkt1->pkt.size;
    SDL_CondSignal(q->cond);

    SDL_UnlockMutex(q->mutex);
    return 0;
}



/* 
static void packet_queue_flush(PacketQueue *q)
{
    MyAVPacketList *pkt, *pkt1;

    SDL_LockMutex(q->mutex);
    for (pkt = q->first_pkt; pkt; pkt = pkt1) {
        pkt1 = pkt->next;
        av_packet_unref(&pkt->pkt);
        av_freep(&pkt);
    }
    q->last_pkt = NULL;
    q->first_pkt = NULL;
    q->nb_packets = 0;
    q->size = 0;
    q->duration = 0;
    SDL_UnlockMutex(q->mutex);
}
*/



// Pull packet from queue.
// May be blocket until packet received or just continue without blocking.
int packet_queue_get(PacketQueue *q, AVPacket *pkt, int block) {
    AVPacketList *pkt1;
    int ret;

    SDL_LockMutex(q->mutex);

    for(;;) {
        if(quit) {
            ret = -1;
            break;
        }
        pkt1 = q->first_pkt;
        if(pkt1) {//not null
            q->first_pkt = pkt1 ->next;
            if(!q->first_pkt)
                q->last_pkt = NULL;
            q->nb_packets--;
            q->size -= pkt1->pkt.size;
            *pkt = pkt1->pkt;
            av_free(pkt1);
            ret = 1;
            break;
        } else if(!block) {
            ret = 0;
            break;
        } else {
            // wait only when there still has data
            // TODO there may be problem with this solution due to updating
            //  put_all without ensuring in multithread program
            if(put_all!=1)
                SDL_CondWait(q->cond, q->mutex);
            else {
                ret = -2;
                break;
            }
        }
    }

    SDL_UnlockMutex(q->mutex);
    return ret;
}
static int open_input_file(const char *filename) {
    int ret;
    unsigned int i;

    ifmt_ctx = NULL;
    if ((ret = avformat_open_input(&ifmt_ctx, filename, NULL, NULL)) < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot open input file\n");
        return ret;
    }

    if ((ret = avformat_find_stream_info(ifmt_ctx, NULL)) < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot find stream information\n");
        return ret;
    }

    stream_ctx = static_cast<StreamContext *>(av_mallocz_array(ifmt_ctx->nb_streams, sizeof(*stream_ctx)));
    if (!stream_ctx)
        return AVERROR(ENOMEM);

    for (i = 0; i < ifmt_ctx->nb_streams; i++) {
        AVStream *stream = ifmt_ctx->streams[i];
        if(stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
            video_stream_indx = i;
        else if(stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
            audio_stream_indx = i;

        AVCodec *dec = avcodec_find_decoder(stream->codecpar->codec_id);
        AVCodecContext *codec_ctx;
        if (!dec) {
            av_log(NULL, AV_LOG_ERROR, "Failed to find decoder for stream #%u\n", i);
            return AVERROR_DECODER_NOT_FOUND;
        }
        codec_ctx = avcodec_alloc_context3(dec);
        if (!codec_ctx) {
            av_log(NULL, AV_LOG_ERROR, "Failed to allocate the decoder context for stream #%u\n", i);
            return AVERROR(ENOMEM);
        }

        ret = avcodec_parameters_to_context(codec_ctx, stream->codecpar);
        if (ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "Failed to copy decoder parameters to input decoder context "
                    "for stream #%u\n", i);
            return ret;
        }
        /* Reencode video & audio and remux subtitles etc. */
        if (codec_ctx->codec_type == AVMEDIA_TYPE_VIDEO
            || codec_ctx->codec_type == AVMEDIA_TYPE_AUDIO) {
            if (codec_ctx->codec_type == AVMEDIA_TYPE_VIDEO)
                codec_ctx->framerate = av_guess_frame_rate(ifmt_ctx, stream, NULL);

                printf("time base %d /%d \n",codec_ctx->time_base.num, codec_ctx->time_base.den);

            /* Open decoder */
            ret = avcodec_open2(codec_ctx,  dec, NULL);
            if (ret < 0) {
                av_log(NULL, AV_LOG_ERROR, "Failed to open decoder for stream #%u\n", i);
                return ret;
            }
        }
        stream_ctx[i].dec_ctx = codec_ctx;
    }

    av_dump_format(ifmt_ctx, 0, filename, 0);
    return 0;
}

int audio_decode_frame(AVCodecContext * acodec_ctx, uint8_t *audio_buf, int buf_size) {
    static AVPacket pkt;
    static AVFrame *frame, *reframe;
    int data_size = 0, ret;

    while (true) {
        // get next packet
        if (packet_queue_get(&audioq, &pkt, 1) < 0) {// no more packet
            printf("Play audio completed, try shutdown\n");
            quit = 1; // stop program
            return -1;
        }
	if(!is_paused)
	{
		ret = avcodec_send_packet(acodec_ctx, &pkt);
		if (ret < 0) {
			printf("decoding failed: %s \n", av_err2str(ret));
			/* if error, skip frame */
			return -1;
		} else {
			while (ret >= 0) {
				frame = av_frame_alloc();
                		ret = avcodec_receive_frame(acodec_ctx, frame);
                		if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                    		printf("Decoding audio: %s \n", av_err2str(ret));
                    		break;
				} else {
					reframe = av_frame_alloc();
                    			reframe->channel_layout = frame->channel_layout;
					reframe->sample_rate = frame->sample_rate;
					reframe->format = OUT_SAMPLE_FMT;
					int swr_state = swr_convert_frame(swr, reframe, frame);
					if (swr_state < 0) {
						// convert error, try next frame
						printf("Audio convert error, try next frame\n");
						continue;
					}
					data_size += av_samples_get_buffer_size(NULL, reframe->channels,
							reframe->nb_samples,
							static_cast<AVSampleFormat>(reframe->format), 0);
					memcpy(audio_buf, reframe->data[0], static_cast<size_t>(data_size));
					av_frame_free(&reframe);
					av_frame_free(&frame);
				}
			}
		}
	}

        if(pkt.data) {// not null
            av_packet_unref(&pkt);
        }
        if(quit) {
            return -1;
        }

        if (data_size)
            return data_size;

    }
}
void audio_callback(void *userdata, Uint8 *stream, int len) {

    AVCodecContext *aCodecCtx = (AVCodecContext *)userdata;
    int len1, audio_size;

    static uint8_t audio_buf[(MAX_AUDIO_FRAME_SIZE * 3) / 2];
    static unsigned int audio_buf_size = 0;
    static unsigned int audio_buf_index = 0;

    while(len > 0) {
        if(audio_buf_index >= audio_buf_size) {
            /* We have already sent all our data; get more */
            audio_size = audio_decode_frame(aCodecCtx, audio_buf, sizeof(audio_buf));
            if(audio_size < 0) {
                printf("no more audio\n");
                /* If error, output silence */
                audio_buf_size = 1024; // arbitrary?
                memset(audio_buf, 0, audio_buf_size);
                quit = 1;
                SDL_CloseAudio();
                return;
            } else {
                audio_buf_size = audio_size;
            }
            audio_buf_index = 0;
        }
        len1 = audio_buf_size - audio_buf_index;
        if(len1 > len)
            len1 = len;
        memcpy(stream, (uint8_t *)audio_buf + audio_buf_index, len1);
        len -= len1;
        stream += len1;
        audio_buf_index += len1;
    }
}

static int init_sdl(){

    // create audio resampling, for converting audio from X format
    // to compatible format with SDL2
    swr = swr_alloc_set_opts(NULL,  // we're allocating a new context
                             stream_ctx[audio_stream_indx].dec_ctx->channel_layout,  // out_ch_layout
                             OUT_SAMPLE_FMT,    // out_sample_fmt
                             stream_ctx[audio_stream_indx].dec_ctx->sample_rate,                // out_sample_rate
                             stream_ctx[audio_stream_indx].dec_ctx->channel_layout, // in_ch_layout: stereo, blabla
                             stream_ctx[audio_stream_indx].dec_ctx->sample_fmt,   // in_sample_fmt
                             stream_ctx[audio_stream_indx].dec_ctx->sample_rate,                // in_sample_rate
                             0,                    // log_offset
                             NULL);
    swr_init(swr);

    /*
     * SDL_Init() essentially tells the library what features we're going to use.
     * SDL_GetError(), of course, is a handy debugging function.
     */
    if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
        fprintf(stderr, "Could not initialize SDL - %s\n", SDL_GetError());
        exit(1);
    }

    // Basically, SDL2 creates a window with a title, then a surface attached to it.
    window = SDL_CreateWindow("StreamyPI",
                              SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                              stream_ctx[video_stream_indx].dec_ctx->width,
                              stream_ctx[video_stream_indx].dec_ctx->height, SDL_WINDOW_SHOWN);

    if (window == NULL) {
        fprintf(stderr, "Window could not be created: %s\n", SDL_GetError());
        return 1;
    }



    renderer = SDL_CreateRenderer(window, -1, 0);
    if (!renderer) {
        fprintf(stderr, "SDL: could not create renderer - exiting\n");
        exit(1);
    }

    // Allocate a place to put our YUV image on that screen
    texture = SDL_CreateTexture(
            renderer,
            SDL_PIXELFORMAT_YV12,
            SDL_TEXTUREACCESS_STREAMING,
            stream_ctx[video_stream_indx].dec_ctx->width,
            stream_ctx[video_stream_indx].dec_ctx->height
    );

    if (!texture) {
        fprintf(stderr, "SDL: could not create texture - exiting\n");
        exit(1);
    }



    // initialize SWS context for software scaling
    sws_ctx = sws_getContext(stream_ctx[video_stream_indx].dec_ctx->width,
                             stream_ctx[video_stream_indx].dec_ctx->height,
                             stream_ctx[video_stream_indx].dec_ctx->pix_fmt,
                             stream_ctx[video_stream_indx].dec_ctx->width,
                             stream_ctx[video_stream_indx].dec_ctx->height,
                             AV_PIX_FMT_YUV420P,
                             SWS_BILINEAR,
                             NULL,
                             NULL,
                             NULL
    );


    // set up YV12 pixel array (12 bits per pixel)
    yPlaneSz = stream_ctx[video_stream_indx].dec_ctx->width * stream_ctx[video_stream_indx].dec_ctx->height;
    uvPlaneSz = stream_ctx[video_stream_indx].dec_ctx->width * stream_ctx[video_stream_indx].dec_ctx->height / 4;
    yPlane = (Uint8*)malloc(yPlaneSz);
    uPlane = (Uint8*)malloc(uvPlaneSz);
    vPlane = (Uint8*)malloc(uvPlaneSz);
    if (!yPlane || !uPlane || !vPlane) {
        fprintf(stderr, "Could not allocate pixel buffers - exiting\n");
        exit(1);
    }

    uvPitch = stream_ctx[video_stream_indx].dec_ctx->width / 2;


    // SDL audio init.
    SDL_memset(&wanted_spec, 0, sizeof(wanted_spec));
    wanted_spec.freq = stream_ctx[audio_stream_indx].dec_ctx->sample_rate ;
    wanted_spec.format = AUDIO_F32LSB;
    wanted_spec.channels = static_cast<Uint8>(stream_ctx[audio_stream_indx].dec_ctx->channels);
    // silence is 0 because it is signed format (+ve, -ve, 0)
    wanted_spec.silence = 0;
    wanted_spec.samples = SDL_AUDIO_BUFFER_SIZE;
    wanted_spec.callback = audio_callback;
    // SDL will give our callback a void pointer to any user data that we want our callback function to have.
    wanted_spec.userdata = stream_ctx[audio_stream_indx].dec_ctx;
    if(SDL_OpenAudio(&wanted_spec, &spec) < 0) {
        fprintf(stderr, "SDL_OpenAudio: %s\n", SDL_GetError());
        exit(1);
    }

}

void event_handler(SDL_Event event, AVFrame* frame){
	
        switch (event.type) {
            case SDL_QUIT:
                printf("SDL receive stop, try shutdown\n");
                quit = 1;
                break;
	    case SDL_KEYDOWN:
		if (event.key.keysym.sym == SDLK_ESCAPE || event.key.keysym.sym == SDLK_q) {
			printf("DAWOD :*****************************************************************QUIT\n");
			do_quit(frame);
			break;
		}
		switch (event.key.keysym.sym) {
			case SDLK_SPACE:
			case SDLK_p:
				if (!is_paused){
					printf("DAWOD :************************************************************* PAUSE\n");
				}
				else{
					printf("DAWOD :************************************************************* RESUME\n");
				}
				is_paused = is_paused ^ 1;

				break;
			case SDLK_m:
				printf("DAWOD :************************************************************** MUTE\n");
				break;
			case SDLK_LEFT:
//				incr = seek_interval ? -seek_interval : -10.0;
				printf("DAWOD :************************************************************** SEEK BACK\n");
				break;
			case SDLK_RIGHT:
//				incr = seek_interval ? seek_interval : 10.0;
				printf("DAWOD :************************************************************** SEEK FORWARD\n");
				break;
			default:
				break;
		}
            default:
                break;
        }
}



int main(int argc, char **argv) {
    // Initialize libav.
    av_register_all();
    uint8_t *buffer = NULL;
    int numBytes;
    int ret;
    AVPacket *packet = av_packet_alloc();
    AVFrame *frame = nullptr;
    AVFrame *pFrameRGB = nullptr;
    int stream_index;
    enum AVMediaType type;

    ret = open_input_file("rtsp://127.0.0.1:8000/gamar.mkv");
    //ret = open_input_file("/home/dawod/Videos/bazringosh.mp4");
    if (ret < 0) {
        printf("couldn't open input file\n");
        exit(1);
    }

    // Init sdl lib.
    init_sdl();

    packet_queue_init(&audioq);
    SDL_PauseAudio(0);


    // Allocate an AVFrame structure
    pFrameRGB = av_frame_alloc();
    if (pFrameRGB == NULL)
        return -1;

    // Determine required buffer size and allocate buffer
    numBytes = av_image_get_buffer_size(AV_PIX_FMT_RGB24, stream_ctx[video_stream_indx].dec_ctx->width,
                                        stream_ctx[video_stream_indx].dec_ctx->height,
                                        16); // add +1 for height and width.

    buffer = (uint8_t *) av_malloc(numBytes * sizeof(uint8_t));

    av_image_fill_arrays(pFrameRGB->data, pFrameRGB->linesize, buffer, AV_PIX_FMT_RGB24,
                         stream_ctx[video_stream_indx].dec_ctx->width, stream_ctx[video_stream_indx].dec_ctx->height,
                         1);

    int i = 0;
    // Read all packets.
    while (quit != 1) {
        printf("in loop \n");
	SDL_PollEvent(&event);
	printf("dawod : 1\n");
	SDL_PumpEvents();
	printf("dawod : 2\n");

//    while (!SDL_PeepEvents(event, 1, SDL_GETEVENT, SDL_FIRSTEVENT, SDL_LASTEVENT)) {
//	    SDL_PumpEvents();
//    }

	event_handler(event, frame);
	printf("dawod : 3\n");


        if (is_paused != last_paused) {
		last_paused = is_paused;
		if (is_paused)
			av_read_pause(ifmt_ctx);
		else
			av_read_play(ifmt_ctx);
        }




        if (is_paused) {
		/*  wait 10 ms to avoid trying to get another packet */
		/*  XXX: horrible */
		SDL_Delay(10);
		continue;
	}
	

/* 
        if (is_seek_req) {
            int64_t seek_target = is->seek_pos;
            int64_t seek_min    = is->seek_rel > 0 ? seek_target - is->seek_rel + 2: INT64_MIN;
            int64_t seek_max    = is->seek_rel < 0 ? seek_target - is->seek_rel - 2: INT64_MAX;
// FIXME the +-2 is due to rounding being not done in the correct direction in generation
//      of the seek_pos/seek_rel variables

            ret = avformat_seek_file(is->ic, -1, seek_min, seek_target, seek_max, is->seek_flags);
            if (ret < 0) {
                av_log(NULL, AV_LOG_ERROR,
                       "%s: error while seeking\n", is->ic->url);
            } else {
                if (is->audio_stream >= 0) {
                    packet_queue_flush(&is->audioq);
                    packet_queue_put(&is->audioq, &flush_pkt);
                }
                if (is->subtitle_stream >= 0) {
                    packet_queue_flush(&is->subtitleq);
                    packet_queue_put(&is->subtitleq, &flush_pkt);
                }
                if (is->video_stream >= 0) {
                    packet_queue_flush(&is->videoq);
                    packet_queue_put(&is->videoq, &flush_pkt);
                }
                if (is->seek_flags & AVSEEK_FLAG_BYTE) {
                   set_clock(&is->extclk, NAN, 0);
                } else {
                   set_clock(&is->extclk, seek_target / (double)AV_TIME_BASE, 0);
                }
            }
            is->seek_req = 0;
            is->queue_attachments_req = 1;
            is->eof = 0;
            if (is->paused)
                step_to_next_frame(is);
        }
*/

	if(!is_paused){
		if ((ret = av_read_frame(ifmt_ctx, packet)) < 0) {
			if(put_all!=1){
				printf("Complete read frames\n");
				put_all = 1;
			}
			sleep(1);
			continue;
		}
		
		if (first_packet_size_read == 0)
			first_packet_size_read = packet->size;
		stream_index = packet->stream_index;
		type = ifmt_ctx->streams[packet->stream_index]->codecpar->codec_type;
		av_log(NULL, AV_LOG_DEBUG, "Demuxer gave frame of stream_index %u\n",
				stream_index);
		
		av_packet_rescale_ts(packet,
				ifmt_ctx->streams[stream_index]->time_base,
				stream_ctx[stream_index].dec_ctx->time_base);
	}
	
	
	
	if (type == AVMEDIA_TYPE_VIDEO) {
		largest_input_packet_size = std::max(largest_input_packet_size, packet->size);

	    
		if(!is_paused)
		{
			ret = avcodec_send_packet(stream_ctx[stream_index].dec_ctx, packet);
			if (ret < 0) {
				av_frame_free(&frame);
				av_log(NULL, AV_LOG_ERROR, "Decoding failed\n");
				break;
			} 
			else 
			{
				// Decode.
				while (ret >= 0) {
				    frame = av_frame_alloc();
				    ret = avcodec_receive_frame(stream_ctx[stream_index].dec_ctx, frame);
				    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
					    break;
				    else if (ret < 0) {
					    fprintf(stderr, "Error during decoding\n");
					    exit(1);
				    }
				    
				    
				    received_frames++;
				    
				    AVFrame pict;
				    pict.data[0] = yPlane;
				    pict.data[1] = uPlane;
				    pict.data[2] = vPlane;
				    pict.linesize[0] = stream_ctx[stream_index].dec_ctx->width;
				    pict.linesize[1] = uvPitch;
				    pict.linesize[2] = uvPitch;
				    
				    /* convert to destination format */
				    sws_scale(sws_ctx, (const uint8_t *const *) frame,
						    frame->linesize, 0, stream_ctx[stream_index].dec_ctx->height,
						    pict.data, pict.linesize);
				    
				    
				    SDL_UpdateYUVTexture(
					    texture,
					    NULL,
					    yPlane,
					    stream_ctx[stream_index].dec_ctx->width,
					    uPlane,
					    uvPitch,
					    vPlane,
					    uvPitch
						    );
				    
				    SDL_RenderClear(renderer);
				    SDL_RenderCopy(renderer, texture, NULL, NULL);
				    SDL_RenderPresent(renderer);
		    
				    
				    printf("showing frame:%d \n", ++received_frames);
				    
				    av_frame_free(&frame);
				    if (ret < 0) {
					    printf("badddd\n");
					    exit(1);
				    }
			    }
		    }
	    }
	
	} 
	else if (type == AVMEDIA_TYPE_AUDIO) {
		packet_queue_put(&audioq, packet);
	}

    }
    SDL_Quit();
    av_frame_free(&frame);
    swr_free(&swr);

    av_frame_free(&frame);
    for (int i = 0; i < ifmt_ctx->nb_streams; i++) {
        avcodec_free_context(&stream_ctx[i].dec_ctx);
        if (ofmt_ctx && ofmt_ctx->nb_streams > i && ofmt_ctx->streams[i] && stream_ctx[i].enc_ctx)
            avcodec_free_context(&stream_ctx[i].enc_ctx);
    }

    av_free(stream_ctx);
    avformat_close_input(&ifmt_ctx);

    return 0;

}

