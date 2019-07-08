#include <QGuiApplication>
#include <QQuickView>
#include "player.h"

int main(int argc, char* argv[])
{

    QGuiApplication app(argc,argv);
    qmlRegisterType<Player>("Player",1,0,"Player");
    QQuickView view;
    view.setResizeMode(QQuickView::SizeRootObjectToView);
    view.setSource(QUrl::fromLocalFile(QLatin1String("Xtream.qml")));

    const QString lowerArgument = QString::fromLatin1(argv[1]).toLower();
    if (lowerArgument == QLatin1String("--fullscreen")) {
        view.showFullScreen();
    } else {
        view.show();
//        //-----------------------------------------------------------
//        //I aded this for testing only
//        Player player;
//        player.play("rtsp://127.0.0.1:8554/baz.mkv",0);
//        //-------------------you must remove these lines--------------------
    }
    return app.exec();
}
