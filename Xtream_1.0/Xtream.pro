TEMPLATE = app

QT += qml quick
SOURCES += main.cpp \
    player.cpp \
    transcoding.cpp


QMAKE_CXXFLAGS += -fprofile-arcs -ftest-coverage

LIBS += \
        -L/usr/lib -lSDL2 \
        -L/usr/local/lib -lavcodec \
        -L/usr/local/lib -lavformat \
        -L/usr/local/lib -lavutil \
        -L/usr/local/lib -lavresample \
        -L/usr/local/lib -lswscale \
        -L/usr/local/lib -lswresample \
        -L/usr/local/lib -lgtest \
        -L/usr/local/lib -lgtest_main \
        -L/usr/local/lib -lgcov



INCLUDEPATH += \
            SDL2/SDL.h SDL2/SDL_thread.h \
            libavcodec/avcodec.h  \
            libavformat/avformat.h \
            libavutil/avutil.h libavutil/timestamp.h libavutil/opt.h \
            libavresample/avresample.h \
            libswscale/swscale.h \
            libswresample/swresample.h


target.path = /opt/Xtream
qml.files = Xtream.qml content
qml.path = /opt/Xtream
INSTALLS += target qml

HEADERS += \
    player.h \
    transcoding.h

