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



void test_video_clock(string film_name){
	
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
	ASSERT_EQ((int)(get_video_clock(videoState)*1000000)  , (int)(videoState->video_current_pts + ((av_gettime() - videoState->video_current_pts_time/1000000000000000000))));

}




void test_external_clock(string film_name){
	
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
	videoState->external_clock_time = av_gettime();
        videoState->external_clock = videoState->external_clock_time / 1000000.0;

	ASSERT_EQ((int)(get_external_clock(videoState)),(int)(av_gettime()/1000000.0));

}






void test_synchronize_video(string film_name){

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






TEST_F(Player_test,test_synchronize_video){
	test_synchronize_video("rtsp://127.0.1.1:8554/baz.mkv"); //problem
}


TEST_F(Player_test,test_video_clock){
	test_video_clock("rtsp://127.0.1.1:8554/baz.mkv");
}
TEST_F(Player_test,test_external_clock){
	test_external_clock("rtsp://127.0.1.1:8554/baz.mkv");
}




int main(int argc, char **argv) {
	  testing::InitGoogleTest(&argc, argv);
	    return RUN_ALL_TESTS();
}



