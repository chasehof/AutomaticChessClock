import QtQuick 2.15
import QtQuick.Controls 2.15

Item {
    signal continue1()
    signal continue2()

    Rectangle {
        anchors.fill: parent
        color: "#101010"

        Column {
            anchors.centerIn: parent
            spacing: 30

            Button {
                text: "Manual"
                width: 200
                height: 80
                onClicked: continueManual()
            }

            Button {
                text: "Auto"
                width: 200
                height: 80
                onClicked: continueAuto()
            }
        }
    }
}