#include <gtest/gtest.h>
#include <stdio.h>
#include <stdlib.h>
#include "../player.h"
#include <string>

using namespace std;



void test_decoder(string film_name){
	VideoState * videoState = NULL;
	av_register_all();
	avformat_network_init();
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER);

	videoState = static_cast<VideoState *>(av_mallocz(sizeof(VideoState)));
	videoState->filename = film_name;
	videoState->pictq_mutex = SDL_CreateMutex();
	videoState->pictq_cond = SDL_CreateCond();
	create_window(videoState, "SuperStreaming|TV", 500, 500);
	schedule_refresh(videoState, 1);
	videoState->av_sync_type = DEFAULT_AV_SYNC_TYPE;

	ASSERT_EQ(decode_thread(videoState) ,0);
}


TEST (single_Test_case,test_decoder){
	  test_decoder("rtsp://127.0.1.1:8554/baz.mkv");
}




int main(int argc, char **argv) {
	  testing::InitGoogleTest(&argc, argv);
	    return RUN_ALL_TESTS(); 
}



