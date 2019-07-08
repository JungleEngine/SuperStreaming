import QtQuick 2.0
import "content"
import Player 1.0

Item {
    id: mainWindow

    width: 620
    height: 720

    QtObject {
        id: settings
        // These are used to scale fonts according to screen size
        property real _scaler: 400 + mainWindow.width * mainWindow.height * 0.00015
        property int fontXS: _scaler * 0.032
        property int fontS: _scaler * 0.040
        property int fontM: _scaler * 0.046
        property int fontMM: _scaler * 0.064
        property int fontL: _scaler * 0.100
        // Settings
        property bool showFogParticles: false
        property bool showShootingStarParticles: false
        property bool showLighting: false
        property bool showColors: true

        property bool no_interpolation: true
        property bool perfect_interpolation: false
        property bool runtime_interpolation: false
        property bool player_type: true

    }

    MainView {
        id: mainView
    }

    InfoView {
        id: infoView
    }

    DetailsView {
        id: detailsView
    }

    PlayerView {
        id: playerview
    }

//    Player {
//        id : player;
//    }



    MoviesModel {
        id: moviesModel
    }
}
