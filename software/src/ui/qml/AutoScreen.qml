import QtQuick 2.15
import QtQuick.Controls 2.15

Item {
    Column {
        anchors.centerIn: parent
        spacing: 30

        // Timer Display
        Rectangle {
            width: 400
            height: 140
            color: "#202020"
            radius: 12

         Text {
                    text: backend.timerA
                    font.pixelSize: 36
                }
        spacing: 10

         Text {
                    text: backend.timerB
                    font.pixelSize: 36
                }
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