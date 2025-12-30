import QtQuick 2.15
import QtQuick.Controls 2.15

ApplicationWindow {
    width: 800
    height: 480
    visible: true
    title: "Qt5 Raspberry Pi UI"

    Row {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 20

        // Left Button
        Button {
            text: "Left"
            width: 150
            height: 80
            onClicked: backend.startTimer()
        }

 Rectangle {
                width: 300
                height: 80
                color: "#202020"
                radius: 10

                Text {
                    anchors.centerIn: parent
                    text: backend.formattedTime
                    color: "white"
                    font.pixelSize: 32
                }
            }

        // Right Button
        Button {
            text: "Right"
            width: 150
            height: 80
            onClicked: backend.stopTimer()
        }

        // Back Button
        Button {
            text: "Back"
            width: 160
            height: 60
            onClicked: StackView.view.pop()
        }
    }
}
