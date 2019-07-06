import QtQuick 2.0

Item {
    id: root

    property bool isShown: false

    anchors.fill: parent

    function show() {
        isShown = true;
        hideAnimation.stop();
        showAnimation.restart();
    }
    function hide() {
        isShown = false;
        showAnimation.stop();
        hideAnimation.restart();
    }

    SequentialAnimation {
        id: showAnimation
        PropertyAction { target: backgroundItem; property: "visible"; value: true }
        ParallelAnimation {
            NumberAnimation { target: backgroundItem; property: "opacity"; to: 1; duration: 250; easing.type: Easing.InOutQuad }
            NumberAnimation { target: backgroundItem; property: "scale"; to: 1; duration: 500; easing.type: Easing.OutBack }
        }
    }
    SequentialAnimation {
        id: hideAnimation
        ParallelAnimation {
            NumberAnimation { target: backgroundItem; property: "opacity"; to: 0; duration: 500; easing.type: Easing.InOutQuad }
            NumberAnimation { target: backgroundItem; property: "scale"; to: 0.6; duration: 500; easing.type: Easing.InOutQuad }
        }
        PropertyAction { target: backgroundItem; property: "visible"; value: false }
    }

    MouseArea {
        anchors.fill: parent
        enabled: root.isShown
        onClicked: {
            root.hide();
        }
    }

    Image {
        id: settingsIcon
        anchors.left: parent.left
        anchors.leftMargin: 4
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 4
        source: "images/settings.png"
        opacity: backgroundItem.opacity + 0.4
        MouseArea {
            anchors.fill: parent
            anchors.margins: -20
            onClicked: {
                if (root.isShown) {
                    root.hide();
                } else {
                    root.show();
                }
            }
        }
    }

    BorderImage {
        id: backgroundItem
        anchors.left: settingsIcon.horizontalCenter
        anchors.bottom: settingsIcon.verticalCenter
        width: Math.min(480, parent.width - 60)
        height: settingsContentColumn.height + 36
        source: "images/panel_bg.png"
        border.left : 22
        border.right : 10
        border.top : 5
        border.bottom : 26

        transformOrigin: Item.BottomLeft
        visible: false
        opacity: 0
        scale: 0.6

        Column {
            id: settingsContentColumn
            width: parent.width
            y: 8
            Switch {
                id : no
                text: "Play skipped video"
                checked: settings.no_interpolation
                onCheckedChanged: {
                    settings.no_interpolation = checked;
                    settings.perfect_interpolation = false
                    perfect.checked = false
                    settings.runtime_interpolation = false
                    runtime.checked = false



                    print("no_interpolation  : " + settings.no_interpolation);
                    print("perfect_interpolation  : " + settings.perfect_interpolation);
                    print("runtime_interpolation  : " + settings.runtime_interpolation);

                }
            }
            Rectangle {
                anchors.horizontalCenter: parent.horizontalCenter
                width: parent.width - 32
                height: 1
                color: "#404040"
            }
            Switch {
                id : perfect
                text: "Perfect Interpolation"
                checked: settings.perfect_interpolation
                onCheckedChanged: {
                    settings.perfect_interpolation = checked;
                    settings.no_interpolation = false
                    no.checked = false
                    settings.runtime_interpolation = false
                    runtime.checked = false

                    print("no_interpolation  : " + settings.no_interpolation);
                    print("perfect_interpolation  : " + settings.perfect_interpolation);
                    print("runtime_interpolation  : " + settings.runtime_interpolation);
                }
            }
            Rectangle {
                anchors.horizontalCenter: parent.horizontalCenter
                width: parent.width - 32
                height: 1
                color: "#404040"
            }
            Switch {
                id : runtime
                text: "Runtime Interpolation"
                checked: settings.runtime_interpolation
                onCheckedChanged: {
                    settings.runtime_interpolation = checked;
                    settings.perfect_interpolation = false
                    perfect.checked = false
                    settings.no_interpolation = false
                    no.checked = false

                    print("no_interpolation  : " + settings.no_interpolation);
                    print("perfect_interpolation  : " + settings.perfect_interpolation);
                    print("runtime_interpolation  : " + settings.runtime_interpolation);
                }
            }
            Rectangle {
                anchors.horizontalCenter: parent.horizontalCenter
                width: parent.width - 32
                height: 1
                color: "#404040"
            }
            Switch {
                text: "Do you l-o-v-e colors?"
                checked: settings.showColors
                onText: "Yes"
                offText: "No!"
                onCheckedChanged: {
                    settings.showColors = checked;
                }
            }
            Rectangle {
                anchors.horizontalCenter: parent.horizontalCenter
                width: parent.width - 32
                height: 1
                color: "#404040"
            }
            Switch {
                text: "Choose Player"
                checked: settings.player_type
                onText: "FF"
                offText: "Xtream"
                onCheckedChanged: {
                    settings.player_type = checked;
                }
            }
//            Rectangle {
//                anchors.horizontalCenter: parent.horizontalCenter
//                width: parent.width - 32
//                height: 1
//                color: "#404040"
//            }
//            Switch {
//                text: "Bye Bye"
//                checked: settings.showColors
//                onText: "NO"
//                offText: "YES"
//                onCheckedChanged: {
//                    //Qt.quit()
//                    //Qt.callLater(Qt.quit)


//                }

//            }
        }
    }
}
