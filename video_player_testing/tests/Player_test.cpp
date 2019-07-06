#include <gtest/gtest.h>
#include <stdio.h>
#include <stdlib.h>
#include "../player.h"


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
		videoState->filename = "rtsp://127.0.1.1:8554/baz.mkv";
		
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
			    
	}
	
	void TearDown() override {}


		
void test_SDL_start(){
	
	ASSERT_EQ(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER), 0);
}

void test_queue_init(){

	int queue_size = sizeof(videoState->audioq);
	packet_queue_init(&videoState->audioq);
	ASSERT_NE(&videoState->audioq, nullptr);
	ASSERT_EQ(queue_size , sizeof(videoState->audioq));
	ASSERT_EQ(videoState->audioq.size , 0);
}

void test_queue_put(){
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


void test_video_clock(){
	
	ASSERT_EQ((int)(get_video_clock(videoState)*1000000)  , (int)(videoState->video_current_pts + ((av_gettime() - videoState->video_current_pts_time/1000000000000000000))));

}




void test_external_clock(){
	
	videoState->external_clock_time = av_gettime();
        videoState->external_clock = videoState->external_clock_time / 1000000.0;

	ASSERT_EQ((int)(get_external_clock(videoState)),(int)(av_gettime()/1000000.0));

}

void test_alloc_pict(){
	alloc_picture(videoState);

	VideoPicture * videoPicture;
	videoPicture = &videoState->pictq[videoState->pictq_windex];	
	
	ASSERT_EQ(videoPicture->width , videoState->window_width);
	ASSERT_EQ(videoPicture->height , videoState->window_height);
	ASSERT_EQ(videoPicture->allocated , 1);
}

void test_queue_pict(){
	
	static AVFrame * pFrame = NULL;
	double pts;
	pFrame = av_frame_alloc();
	VideoPicture * videoPicture;
	videoPicture = &videoState->pictq[videoState->pictq_windex];
	AVPacket * packet = av_packet_alloc();

        int ret = packet_queue_get(&videoState->videoq, packet, 1);
        ret = avcodec_send_packet(videoState->video_ctx, packet);

	int counter = 0;
	while (ret >= 0 && counter++ < 10)
	{
		ret = avcodec_receive_frame(videoState->video_ctx, pFrame);
		pts = guess_correct_pts(videoState->video_ctx, pFrame->pts, pFrame->pkt_dts);
		
		queue_picture(videoState, pFrame, pts);

		
	ASSERT_EQ(videoPicture->pts , pts);
	ASSERT_EQ(videoPicture->frame->pict_type , pFrame->pict_type);
	ASSERT_EQ(videoPicture->frame->pts , pFrame->pts);
	ASSERT_EQ(videoPicture->frame->pkt_dts , pFrame->pkt_dts);
	ASSERT_EQ(videoPicture->frame->key_frame , pFrame->key_frame);
	ASSERT_EQ(videoPicture->frame->coded_picture_number , pFrame->coded_picture_number);
	ASSERT_EQ(videoPicture->frame->display_picture_number , pFrame->display_picture_number);
	ASSERT_EQ(videoPicture->frame->width , videoState->window_width);
	ASSERT_EQ(videoPicture->frame->height , videoState->window_height);
	}
}





void test_synchronize_video(){

	static AVFrame * pFrame = NULL;
	double pts , ppts, vid_clk;
	pFrame = av_frame_alloc();
	AVPacket * packet = av_packet_alloc();

        int ret = packet_queue_get(&videoState->videoq, packet, 1);
        ret = avcodec_send_packet(videoState->video_ctx, packet);

	while (ret >= 0)
	{
		ret = avcodec_receive_frame(videoState->video_ctx, pFrame);
		pts = guess_correct_pts(videoState->video_ctx, pFrame->pts, pFrame->pkt_dts);
		ppts = pts;
		vid_clk = videoState->video_clock;
		synchronize_video(videoState, pFrame, pts);
	}
	
	int frame_delay = av_q2d(videoState->video_ctx->time_base);

	ASSERT_EQ(pts, ((ppts)?ppts:videoState->video_clock));
	ASSERT_EQ(videoState->video_clock, ((ppts)? ppts+ frame_delay +pFrame->repeat_pict * (frame_delay * 0.5) : vid_clk+pFrame->repeat_pict * (frame_delay * 0.5)+frame_delay ));
}





};


void test_decoder(){
	VideoState * videoState = NULL;
	av_register_all();
	avformat_network_init();
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER);

	videoState = static_cast<VideoState *>(av_mallocz(sizeof(VideoState)));
	videoState->filename = "rtsp://127.0.1.1:8554/baz.mkv";
	videoState->pictq_mutex = SDL_CreateMutex();
	videoState->pictq_cond = SDL_CreateCond();
	create_window(videoState, "SuperStreaming|TV", 500, 500);
	schedule_refresh(videoState, 1);
	videoState->av_sync_type = DEFAULT_AV_SYNC_TYPE;

	ASSERT_EQ(decode_thread(videoState) ,0);
}





//TEST_F(Player_test,test_synchronize_video){
//	test_synchronize_video(); //problem
//}
TEST_F(Player_test,test_queu_picture){
	test_queue_pict();  //problem
}



//TEST_F(Player_test,test_queue_put){
//	test_queue_put(); //confusing
//}
//
//TEST_F(Player_test,test_video_clock){
//	test_video_clock();
//}
//TEST_F(Player_test,test_allocate_picture){
//	test_alloc_pict();
//}
//TEST_F(Player_test,test_external_clock){
//	test_external_clock();
//}
//TEST_F(Player_test,test_SDL_start){
//	  test_SDL_start();
//}
//TEST_F(Player_test,test_queue_init){
//	  test_queue_init();
//}
//TEST (single_Test_case,test_decoder){
//	  test_decoder();
//}




int main(int argc, char **argv) {
	  testing::InitGoogleTest(&argc, argv);
	    return RUN_ALL_TESTS();
}



