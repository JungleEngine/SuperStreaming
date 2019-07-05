#include <gtest/gtest.h>
#include <stdio.h>
#include <stdlib.h>
#include "../player.h"
#include <string>


using namespace std;


class Player_test : public testing::Test{
protected:
	VideoState * videoState = NULL;
	char * pEnd;
	
	void SetUp() override {
		av_register_all();
		avformat_network_init();
		test_SDL_start();
		
		videoState = static_cast<VideoState *>(av_mallocz(sizeof(VideoState)));
			    
	}
	
	void TearDown() override {}


		
void test_SDL_start(){
	
	ASSERT_EQ(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER), 0);
}

void test_queue_init(string film_name){

		videoState->filename = film_name;
		
		videoState->pictq_mutex = SDL_CreateMutex();
		videoState->pictq_cond = SDL_CreateCond();
		create_window(videoState, "SuperStreaming|TV", 500, 500);
		
		schedule_refresh(videoState, 1);
		videoState->av_sync_type = DEFAULT_AV_SYNC_TYPE;

		videoState->decode_tid = SDL_CreateThread(decode_thread, "Decoding Thread", videoState);
		
		if(!videoState->decode_tid)
		{
			printf("Could not start decoding SDL_Thread: %s.\n", SDL_GetError());
			av_free(videoState);
			return ;
		}
	int queue_size = sizeof(videoState->audioq);
	packet_queue_init(&videoState->audioq);
	ASSERT_NE(&videoState->audioq, nullptr);
	ASSERT_EQ(queue_size , sizeof(videoState->audioq));
	ASSERT_EQ(videoState->audioq.size , 0);
}



void test_queue_put(string film_name){


	
		videoState->filename = film_name;
		
		videoState->pictq_mutex = SDL_CreateMutex();
		videoState->pictq_cond = SDL_CreateCond();
		create_window(videoState, "SuperStreaming|TV", 500, 500);
		
		schedule_refresh(videoState, 1);
		videoState->av_sync_type = DEFAULT_AV_SYNC_TYPE;

		videoState->decode_tid = SDL_CreateThread(decode_thread, "Decoding Thread", videoState);
		
		if(!videoState->decode_tid)
		{
			printf("Could not start decoding SDL_Thread: %s.\n", SDL_GetError());
			av_free(videoState);
			return ;
		}

//---------------------------------------------------------------------------------------------




    int ret = 0;

    // file I/O context: demuxers read a media file and split it into chunks of data (packets)
    AVFormatContext * pFormatCtx = NULL;
    ret = avformat_open_input(&pFormatCtx, videoState->filename.c_str(), NULL, NULL);
    EXPECT_TRUE(ret>=0);

    AVPacket pkt1, *packet = &pkt1;

    // reset stream indexes
    videoState->videoStream = -1;
    videoState->audioStream = -1;

    // set global VideoState reference
//    global_video_state = videoState;

    // set the AVFormatContext for the global VideoState reference
    videoState->pFormatCtx = pFormatCtx;

    // read packets of the media file to get stream information
    ret = avformat_find_stream_info(pFormatCtx, NULL);
    EXPECT_TRUE(ret>=0);

    // dump information about file onto standard error
    av_dump_format(pFormatCtx, 0, videoState->filename.c_str(), 0);

    // video and audio stream local indexes
    int videoStream = -1;
    int audioStream = -1;

    // loop through the streams that have been found
    for (int i = 0; i < pFormatCtx->nb_streams; i++)
    {
        // look for video stream
        if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO && videoStream < 0)
        {
            videoStream = i;
        }

        // look for audio stream
        if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO && audioStream < 0)
        {
            audioStream = i;
        }
    }

    // return with error in case no video stream was found
    if (videoStream == -1)
    {
        printf("Could not find video stream.\n");
        ASSERT_TRUE(false);
    }
    else
    {
        // open video stream component codec
        ret = stream_component_open(videoState, videoStream);

        // check video codec was opened correctly
        if (ret < 0)
        {
            printf("Could not open video codec.\n");
            ASSERT_TRUE(false);
        }
    }

    // return with error in case no audio stream was found
    if (audioStream == -1)
    {
        printf("Could not find audio stream.\n");
        ASSERT_TRUE(false);
    }
    else
    {
        // open audio stream component codec
        ret = stream_component_open(videoState, audioStream);

        // check audio codec was opened correctly
        if (ret < 0)
        {
            printf("Could not open audio codec.\n");
	    ASSERT_TRUE(false);
        }
    }

    // check both the audio and video codecs were correctly retrieved
    if (videoState->videoStream < 0 || videoState->audioStream < 0)
    {
        printf("Could not open codecs: %s.\n", videoState->filename);
	ASSERT_TRUE(false);
    }

    // main decode loop: read in a packet and put it on the right queue
    for (;;)
    {
        // check global quit flag
        if (videoState->quit)
        {
            
	    ASSERT_TRUE(false);
        }

        // check audio and video packets queues size
        if (videoState->audioq.size > MAX_AUDIOQ_SIZE || videoState->videoq.size > MAX_VIDEOQ_SIZE)
        {   printf("ERROR| AUDIO size is greater than MAX_SIZE with size_vid:%d size_aud:%d\n", videoState->videoq.size,videoState->audioq.size);
            SDL_Delay(10);
            continue;
        }

        // read data from the AVFormatContext by repeatedly calling av_read_frame()
        if (av_read_frame(videoState->pFormatCtx, packet) < 0)
        {
            if (videoState->pFormatCtx->pb->error == 0)
            {
                printf("ERROR| wait for user input");
                // no read error; wait for user input
                SDL_Delay(10);

             continue;
            }
            else
            {
                // exit for loop in case of error
	    break;
            }
        }

        // put the packet in the appropriate queue
        if (packet->stream_index == videoState->videoStream)
        {
            packet_queue_put(&videoState->videoq, packet);
        }
        else if (packet->stream_index == videoState->audioStream)
        {
            packet_queue_put(&videoState->audioq, packet);
        }
        else
        {
            // otherwise free the memory
            av_packet_unref(packet);
        }
	if(videoState->videoq.size>0)
		break;
    }
//---------------------------------------------------------------------------------------------
	ASSERT_TRUE(true);

	
}


};







TEST_F(Player_test,test_queue_put){
	test_queue_put("rtsp://127.0.1.1:8554/baz.mkv"); //confusing
}



TEST_F(Player_test,test_queue_init){
	  test_queue_init("rtsp://127.0.1.1:8554/baz.mkv");
}


int main(int argc, char **argv) {
	  testing::InitGoogleTest(&argc, argv);
	    return RUN_ALL_TESTS();
}



