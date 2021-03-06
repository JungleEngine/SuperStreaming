#include "player.h"


int main(int argc, char *argv[])
{

    avformat_network_init();
    int ret = -1;

    /**
     * Initialize SDL.
     * New API: this implementation does not use deprecated SDL functionalities.
     */
    ret = SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER);
    if (ret != 0)
    {
        printf("Could not initialize SDL - %s\n.", SDL_GetError());
        return -1;
    }

    // the global VideoState reference will be set in decode_thread() to this pointer
    VideoState * videoState = NULL;

    // allocate memory for the VideoState and zero it out
    videoState = static_cast<VideoState *>(av_mallocz(sizeof(VideoState)));

    // copy the file name input by the user to the VideoState structure
//    "/media/syrix/programms/projects/GP/SuperStreaming/ffmpeg_examples/live555_server/live/mediaServer/GOT.mkv"
//    "rtsp://127.0.1.1:8554/GOT.mkv"
    if(argc<3){
        videoState->filename = "rtsp://127.0.1.1:554/animation_cut_skipped.mkv";
        videoState->index_filename = "/media/syrix/programms/projects/GP/SuperStreaming/video_skipping/animation_cut_skipped.mkv.index";
//        videoState->index_filename = "/";

    }else {
        videoState->filename = argv[1];//"rtsp://127.0.1.1:8554/baz.mkv";
        videoState->index_filename = argv[2];
    }
    videoState->outOFTime = true;
    openIndexFile(videoState);

    // parse max frames to decode input by the user
    char * pEnd;

    // initialize locks for the display buffer (pictq)
    videoState->pictq_mutex = SDL_CreateMutex();
    videoState->pictq_cond = SDL_CreateCond();
    create_window(videoState, "SuperStreaming|TV", 1024, 768);

    // launch our threads by pushing an SDL_event of type FF_REFRESH_EVENT
    schedule_refresh(videoState, 1);
    videoState->av_sync_type = DEFAULT_AV_SYNC_TYPE;

    // start the decoding thread to read data from the AVFormatContext
    videoState->decode_tid = SDL_CreateThread(decode_thread, "Decoding Thread", videoState);
    // check the decode thread was correctly started
    if(!videoState->decode_tid)
    {
        printf("Could not start decoding SDL_Thread: %s.\n", SDL_GetError());

        // free allocated memory before exiting
        av_free(videoState);

        return -1;
    }

    // infinite loop waiting for fired events
    SDL_Event event;
    for(;;)
    {
        // wait indefinitely for the next available event
        ret = SDL_WaitEvent(&event);
        if (ret == 0)
        {
            printf("SDL_WaitEvent failed: %s.\n", SDL_GetError());
        }


        switch(event.key.keysym.sym) {
            case SDL_QUIT:
            case SDLK_ESCAPE:
                printf("ESCAPE_PRESSED\n");
                return 0;
            default:break;
        }

        // switch on the retrieved event type
        switch(event.type)
        {
            case FF_QUIT_EVENT:
            case SDL_QUIT:
            {
                printf("exit\n");
                videoState->quit = 1;
                SDL_Quit();
                return 0;
            }
                break;

            case FF_REFRESH_EVENT:
            {
                video_refresh_timer(event.user.data1);
            }
                break;

            default:
            {
                // nothing to do
            }
                break;
        }

        // check global quit flag
        if (videoState->quit)
        {
            // exit for loop
            break;
        }
    }

    // clean up memory
    av_free(videoState);

    return 0;
}
