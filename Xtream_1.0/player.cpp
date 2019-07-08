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


void Player::play(QString txt, QString indx , bool player_type)
{


    string filmName = txt.toStdString();
    string indexName = indx.toStdString();
    string filmName1 , filmName2;



     filmName1  = "ffplay "+filmName;

     filmName2  = "../video_player/transcoding "+filmName + " " + indexName;


     printf("the path = %s\n",filmName);
     printf("the path = %s\n",indexName);
     printf("the path = %s\n",filmName2);




     if (player_type==1){
         const char * filmNamee = filmName1.c_str();
         system( filmNamee);
     }
     else
     {
         const char * filmNamee = filmName2.c_str();
         printf("the path = %s\n",filmNamee);
         //transcoding(filmNamee);
         system( filmNamee);

     }

}
