
/* ===
; Copyright (c) 1995-present, Dwyco, Inc.
; 
; This Source Code Form is subject to the terms of the Mozilla Public
; License, v. 2.0. If a copy of the MPL was not distributed with this file,
; You can obtain one at https://mozilla.org/MPL/2.0/.
*/
import QtQuick 2.6
import QtQuick.Controls 2.1
import QtQuick.Layouts 1.3

RowLayout {
    property alias text_input: textInput1.text
    property alias placeholder_text: textInput1.placeholderText
    TextField {
        id: textInput1
        placeholderText:  qsTr("Enter nickname (you can change it later)")

        Layout.fillWidth: true
        Layout.alignment: Qt.AlignVCenter
    }
    Button {
        text: "x"
        onClicked: {
            textInput1.text = ""
        }
        Layout.maximumHeight: textInput1.height
        Layout.maximumWidth: textInput1.height
        Layout.alignment: Qt.AlignVCenter
    }

    Layout.fillWidth: true
}
