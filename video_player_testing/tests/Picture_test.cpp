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

void test_alloc_pict(string film_name){

	
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
	alloc_picture(videoState);

	VideoPicture * videoPicture;
	videoPicture = &videoState->pictq[videoState->pictq_windex];	
	
	ASSERT_EQ(videoPicture->width , videoState->window_width);
	ASSERT_EQ(videoPicture->height , videoState->window_height);
	ASSERT_EQ(videoPicture->allocated , 1);
}

void test_queue_pict(string film_name){
	
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
	static AVFrame * pFrame = NULL;
	double pts;
	pFrame = av_frame_alloc();
	VideoPicture * videoPicture;
	videoPicture = &videoState->pictq[videoState->pictq_windex];
	AVPacket * packet = av_packet_alloc();

        int ret = packet_queue_get(&videoState->videoq, packet, 1);
        ret = avcodec_send_packet(videoState->video_ctx, packet);

	int counter = 0;
	while (ret >= 0 && counter++ < 2)
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







};








//TEST_F(Player_test,test_queu_picture_gamar){
//	test_queue_pict("rtsp://127.0.1.1:8554/gamar.mkv");  
//}
//TEST_F(Player_test,test_allocate_picture_gamar){
//	test_alloc_pict("rtsp://127.0.1.1:8554/gamar.mkv");
//}

//TEST_F(Player_test,test_queu_picture_baz){
//	test_queue_pict("rtsp://127.0.1.1:8554/baz.mkv");  
//}
//TEST_F(Player_test,test_allocate_picture_baz){
//	test_alloc_pict("rtsp://127.0.1.1:8554/baz.mkv");
//}

  
TEST_F(Player_test,test_queu_picture_test){
	test_queue_pict("rtsp://127.0.1.1:8554/test.mkv");  
}
TEST_F(Player_test,test_allocate_picture_test){
	test_alloc_pict("rtsp://127.0.1.1:8554/test.mkv");
}




int main(int argc, char **argv) {
	  testing::InitGoogleTest(&argc, argv);
	    return RUN_ALL_TESTS();
}

