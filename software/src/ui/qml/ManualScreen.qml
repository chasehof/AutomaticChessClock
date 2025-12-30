import QtQuick 2.15
import QtQuick.Controls 2.15

Item {
    Column {
        anchors.fill: parent
        spacing: 30

        Button {
            text: "Back"
            width: 120
            height: 50
            onClicked: StackView.view.pop()
        }

        Row {
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: 60

            // LEFT SIDE (Timer A)
            Column {
                spacing: 10

                Text {
                    text: backend.timerA
                    font.pixelSize: 36
                }

                Button {
                    text: "White"
                    width: 140
                    height: 60
                    onClicked: backend.pressLeft()
                }
            }

            // RIGHT SIDE (Timer B)
            Column {
                spacing: 10

                Text {
                    text: backend.timerB
                    font.pixelSize: 36
                }

                Button {
                    text: "Black"
                    width: 140
                    height: 60
                    onClicked: backend.pressRight()
                }
            }
        }
    }
}