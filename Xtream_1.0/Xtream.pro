TEMPLATE = app

QT += qml quick
SOURCES += main.cpp \
    player.cpp \


QMAKE_CXXFLAGS += -fprofile-arcs -ftest-coverage

LIBS += \
         -lSDL2 \
         -lavcodec \
         -lavformat \
         -lavutil \
     #    -lavresample \
         -lswscale \
         -lswresample \
         -lgtest \
         -lgtest_main \
         -lgcov


INCLUDEPATH += \
            SDL2/SDL.h SDL2/SDL_thread.h \
            libavcodec/avcodec.h  \
            libavformat/avformat.h \
            libavutil/avutil.h libavutil/timestamp.h libavutil/opt.h \
        #    libavresample/avresample.h \
            libswscale/swscale.h \
            libswresample/swresample.h


target.path = /opt/Xtream
qml.files = Xtream.qml content
qml.path = /opt/Xtream
INSTALLS += target qml

HEADERS += \
    player.h \
    transcoding.h

