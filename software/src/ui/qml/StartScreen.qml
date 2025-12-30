import QtQuick 2.15
import QtQuick.Controls 2.15

Item {
    signal continueManual()
    signal continueAuto()

    Rectangle {
        anchors.fill: parent
        color: "#101010"

        Column {
            anchors.centerIn: parent
            spacing: 25
            SpinBox {
                   id: timeInput
                   from: 1
                   to: 3600
                   value: 60
                   width: 200
            }
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