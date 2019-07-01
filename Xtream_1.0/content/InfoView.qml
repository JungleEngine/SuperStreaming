import QtQuick 2.0

Item {
    id: root

    property bool isShown: false

    anchors.fill: parent

    QtObject {
        id: priv
        property bool poleOut: false
        // How curly the curtain is when opened
        property int endCurly: 80
        // 0 = pole in, 1 = pole out
        property real polePosition: 0
        property bool showingStarted: false
    }

    function show() {
        priv.showingStarted = true;
        isShown = true;
        hideCurtainAnimation.stop();
        hidePoleAnimation.stop();
        if (priv.poleOut) {
            showCurtainAnimation.restart();
        } else {
            showPoleAnimation.restart();
        }
    }
    function hide() {
        priv.showingStarted = false;
        showCurtainAnimation.stop();
        showPoleAnimation.stop();
        if (priv.poleOut) {
            hideCurtainAnimation.restart();
        } else {
            hidePoleAnimation.restart();
        }
    }

    onIsShownChanged: {
        if (root.isShown) {
            mainView.scheduleUpdate();
        }
    }

    Binding {
        target: mainView
        property: "blurAmount"
        value: 40 * priv.polePosition
        when: root.isShown
    }

    // Pole show/hide animations
    SequentialAnimation {
        id: showPoleAnimation
        NumberAnimation { target: priv; property: "polePosition"; to: 1; duration: 600; easing.type: Easing.InOutQuad }
        PropertyAction { target: priv; property: "poleOut"; value: true }
        ScriptAction { script: showCurtainAnimation.restart(); }
    }
    SequentialAnimation {
        id: hidePoleAnimation
        PropertyAction { target: priv; property: "poleOut"; value: false }
        NumberAnimation { target: priv; property: "polePosition"; to: 0; duration: 600; easing.type: Easing.InOutQuad }
        PropertyAction { target: root; property: "isShown"; value: false }
    }

    // Curtain show/hide animations
    SequentialAnimation {
        id: showCurtainAnimation
        NumberAnimation { target: curtainEffect; property: "rightHeight"; to: root.height-8; duration: 1200; easing.type: Easing.OutBack }
    }
    SequentialAnimation {
        id: hideCurtainAnimation
        NumberAnimation { target: curtainEffect; property: "rightHeight"; to: 0; duration: 600; easing.type: Easing.OutCirc }
        ScriptAction { script: hidePoleAnimation.restart(); }
    }

    MouseArea {
        anchors.fill: parent
        enabled: root.isShown
        onClicked: {
            root.hide();
        }
    }

    BorderImage {
        anchors.right: parent.right
        anchors.top: parent.top
        border.left: 22
        border.right: 64
        border.top: 0
        border.bottom: 0
        width: 86 + priv.polePosition * (viewItem.width-88)
        z: 20
        source: "images/info.png"
        opacity: 0.5 + priv.polePosition
        MouseArea {
            anchors.fill: parent
            anchors.margins: -20
            onClicked: {
                if (priv.showingStarted) {
                    root.hide();
                } else {
                    root.show();
                }
            }
        }
    }

    Item {
        id: viewItem
        anchors.right: parent.right
        width: Math.min(620, parent.width)
        height: parent.height + priv.endCurly - 16
        y: 8
        visible: isShown

        Rectangle {
            id: backgroundItem
            anchors.fill: parent
            anchors.margins: 16
            anchors.topMargin: 8
            gradient: Gradient {
                GradientStop { position: 0.0; color: "#101010" }
                GradientStop { position: 0.3; color: "#404040" }
                GradientStop { position: 1.0; color: "#090909" }
            }
            border.color: "#808080"
            border.width: 1
            opacity: 0.8
        }

        Flickable {
            id: flickableArea
            anchors.fill: backgroundItem
            contentHeight: infoTextColumn.height + 32
            contentWidth: backgroundItem.width
            flickableDirection: Flickable.VerticalFlick
            clip: true

            Column {
                id: infoTextColumn
                width: parent.width
                spacing: 32

                Text {
                    id: textItem
                    x: 32
                    height: 60
                    color: "#ffffff"
                    font.pixelSize: settings.fontMM
                    text: "<i>About US</i>"
                    verticalAlignment: Text.AlignBottom
                }

                InfoViewItem {
                    text: "<i>Xtream</i>​ is a streaming service which addresses the problem of bandwidth efficiency of video streaming. The video is processed at the server side to drop some frames from the video to produce another version of the video which is smaller in size. The smaller version is sent to the client which goes through the frame-interpolation process to recover the dropped frames and reconstruct the original video for the user, thus the network bandwidth is reduced."
                    image: "images/xtream_logo.png"
                }
//                InfoViewItem {
//                    text: "<b></b><br/>We are team of computer Engineers."
//                }
                InfoViewItem {
                    text: "<b>Prof.Nevin</b><br/>Prof.Nevin Darwish <br/>ndarwish@eng.cu.edu.eg <br/>01222247364 <br/>"
                    image: "images/prof.png"
                }
                InfoViewItem {
                    text: "<b>Abdelhady</b><br/>Mohamed Abdelhady Ali <br/>mohamed_abdelhady96@hotmail.com <br/>01013620223 <br/>"
                    image: "images/abdelhady.png"
                    switchedLayout: true
                }
                InfoViewItem {
                    text: "<b>Ali</b><br/>Mohamed Ali Mohamed<br/>muhamedali166@gmail.com <br/>01113250833 <br/>"
                    image: "images/ali.png"

                }
                InfoViewItem {
                    text: "<b>Dawod</b><br/>Mohamed Hassan Eldawody <br/>mhm.dawod@gmail.com <br/>01117311726 <br/>"
                    image: "images/dawod.png"
                    switchedLayout: true
                }
                InfoViewItem {
                    text: "<b>Samir</b><br/>Ahmed Samir Ibrahim  <br/> samir97@hotmail.com   <br/> 01147592448 <br/>"
                    image: "images/samir.png"

                }


                Row {
                    anchors.horizontalCenter: parent.horizontalCenter
                    height: 128
                    spacing: 16
                    Image {
                        source: "images/xtream_logo_2.png"
                        anchors.bottom: parent.bottom
                    }

                }

            }
        }

        // Grip to close the view by flicking
        Image {
            id: gripImage
            anchors.right: parent.right
            anchors.rightMargin: 8
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 8
            source: "images/grip.png"
            opacity: 0.25
            MouseArea {
                property int pressedY: 0
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.bottomMargin: priv.endCurly - 16
                anchors.margins: -16
                width: 90
                height: 80
                onPressed: {
                    showCurtainAnimation.stop();
                    hideCurtainAnimation.stop();
                    pressedY = mouseY
                }
                onPositionChanged: {
                    curtainEffect.rightHeight = root.height - (pressedY - mouseY) - 8
                }
                onReleased: {
                    if (mouseY < -root.height*0.2) {
                        root.hide();
                    } else {
                        root.show();
                    }
                }
            }
        }
    }

    CurtainEffect {
        id: curtainEffect
        anchors.fill: viewItem
        source: ShaderEffectSource { sourceItem: viewItem; hideSource: true }
        rightHeight: 0
        leftHeight: rightHeight
        Behavior on leftHeight {
            SpringAnimation { spring: .4; damping: .05; mass: .5 }
        }
        // Hide smoothly when curtain closes
        opacity: 0.004 * rightHeight
    }
}
