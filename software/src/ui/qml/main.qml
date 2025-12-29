import QtQuick 2.15
import QtQuick.Controls 2.15

ApplicationWindow {
    width: 800
    height: 480
    visible: true
    title: "Qt5 Raspberry Pi UI"

    StackView {
        id: stack
        anchors.fill: parent

        initialItem: StartScreen {
            onContinueManual: stack.push("qrc:/qml/ManualScreen.qml")
            onContinueAuto: stack.push("qrc:/qml/AutoScreen.qml")
        }
    }
}