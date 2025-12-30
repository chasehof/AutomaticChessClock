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
            
        Text {
            text: "Set Timer"
            font.pixelSize: 24
        }

        Row {
            spacing: 10

            SpinBox {
                id: minutesInput
                from: 0
                to: 99
                value: 1
                width: 100
            }

            Text { text: "min" }

            SpinBox {
                id: secondsInput
                from: 0
                to: 59
                value: 0
                width: 100
            }

            Text { text: "sec" }
        }
            Button {
            text: "Start"
            width: 200
            height: 70
            onClicked: {
                backend.setInitialTime(minutesInput.value, secondsInput.value)
                showMain()
            }
            Button {
                text: "Manual"
                width: 200
                height: 80
                onClicked: {
                backend.setInitialTime(minutesInput.value, secondsInput.value)
                continueManual()
              }
            }

            Button {
                text: "Auto"
                width: 200
                height: 80
                onClicked: {
                backend.setInitialTime(minutesInput.value, secondsInput.value)
                continueAuto()
              }
            }
        }
    }
}