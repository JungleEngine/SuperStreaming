#include "player.h"
#include <string.h>
#include <stdio.h>
#include "transcoding.h"
#include <bits/stdc++.h>

using namespace std;


Player::Player(QObject *parent):
QObject(parent)
{
}
//{

////        transcoding("rtsp://127.0.0.1:8554/test.mkv");

//}


void Player::play(QString txt , bool player_type)
{


    string filmName = txt.toStdString();

    printf("string = %s\n",filmName);
    string filmName1  = "/home/dawod/GP/Xtream_1.0/FFmpeg/ffplay "+filmName;
    printf("string = %s\n",filmName1);


     if (player_type==1){
         const char * filmNamee = filmName1.c_str();
         system( filmNamee /*"ffplay rtsp://127.0.0.1:8554/baz.mkv"*/);
     }
     else
     {
         const char * filmNamee = filmName.c_str();
   transcoding(filmNamee);
     }

}
