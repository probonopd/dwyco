
/* ===
; Copyright (c) 1995-present, Dwyco, Inc.
; 
; This Source Code Form is subject to the terms of the Mozilla Public
; License, v. 2.0. If a copy of the MPL was not distributed with this file,
; You can obtain one at https://mozilla.org/MPL/2.0/.
*/
import QtQuick 2.6
import QtGraphicalEffects 1.0
import QtQml 2.2
import QtQuick.Controls 2.1


ToolButton {
    background: Image {
        id: bgimage
        anchors.centerIn: parent
        source : mi("ic_home_black_24dp.png")
    }
    contentItem:  Rectangle {
        anchors.centerIn: parent
        visible: core.unread_count > 0
        color: "indigo"
        anchors.margins: mm(1)
        anchors.fill: parent
        radius: 3
        Text {
            anchors.centerIn: parent
            anchors.fill: parent
            text: core.unread_count > 9 ? "9+" : String(core.unread_count)
            color: "white"
            visible: core.unread_count > 0
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
    }



    checkable: false
    onClicked: {
        // go back to top level
        stack.pop(null)
    }

}
