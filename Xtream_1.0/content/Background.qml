import QtQuick 2.0
import QtQuick.Particles 2.0

Rectangle {
    id: root

    anchors.fill: parent
    color: "#ffffff"

    Image {
        id: backgroundImage
        anchors.fill: parent
        source: settings.showColors ? "images/background3.png" : "images/background.png"
        Behavior on source {
            SequentialAnimation {
                NumberAnimation { target: backgroundImage; property: "opacity"; to: 0; duration: 400; easing.type: Easing.InQuad }
                PropertyAction { target: backgroundImage; property: "source" }
                NumberAnimation { target: backgroundImage; property: "opacity"; to: 1; duration: 400; easing.type: Easing.OutQuad }
            }
        }
    }

    // Sky stars particles
    ParticleSystem {
        width: parent.width
        height: 220
        paused: detailsView.isShown || infoView.isShown || settings.showShootingStarParticles
        ImageParticle {
            source: "images/star.png"
            rotationVariation: 10
        }
        Emitter {
            anchors.fill: parent
            emitRate: 10     //Number of particles emitted per second.
            lifeSpan: 5000  //The time in milliseconds each emitted particle should last for.
            size: 32
            sizeVariation: 16
        }
    }
}
