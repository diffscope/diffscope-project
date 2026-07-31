import QtQuick
import QtQuick.Dialogs

Window {
    id: host

    property url selectedFile
    property bool completed: false

    signal done(var result)

    function complete(result) {
        if (completed)
            return
        completed = true
        done(result)
        close()
    }

    visible: false
    width: 1
    height: 1

    Component.onCompleted: picker.open()

    FileDialog {
        id: picker
        title: qsTr("Select LibreSVIP command-line tool")
        fileMode: FileDialog.OpenFile
        nameFilters: Qt.platform.os === "windows"
                     ? [qsTr("Executable files (*.exe)"), qsTr("All files (*)")]
                     : [qsTr("All files (*)")]
        onAccepted: {
            host.selectedFile = selectedFile
            host.complete("accepted")
        }
        onRejected: host.complete("cancelled")
    }
}
