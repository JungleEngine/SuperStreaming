#ifndef PLAYER_H
#define PLAYER_H

#include <QObject>

class Player : public QObject
{
    Q_OBJECT
public:
   explicit Player(QObject *parent = 0);
    Q_INVOKABLE void play(QString txt, QString indx, bool player_type);

signals:

public slots:

};

#endif // PLAYER_H
