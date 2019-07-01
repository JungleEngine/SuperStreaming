#include "player.h"
#include <string.h>
#include "transcoding.h"
#include <bits/stdc++.h>

Player::Player(QObject *parent):
QObject(parent)
{
}
//{

////        transcoding("rtsp://127.0.0.1:8554/test.mkv");

//}


void Player::play(QString txt , bool player_type)
{
    //transcoding("rtsp://127.0.0.1:8554/test.mkv");
    QByteArray ba2 = txt.toLocal8Bit();
    txt  = "ffplay "+txt;
     QByteArray ba = txt.toLocal8Bit();
     char *filmName = ba.data();
     char *filmName2 = ba2.data();

     //printf("\n\nDAWOD__________________________ %s\n\n",filmName);
     if (player_type==1)
         system( filmName /*"ffplay rtsp://127.0.0.1:8554/baz.mkv"*/);
     else
   transcoding(filmName2);
}
