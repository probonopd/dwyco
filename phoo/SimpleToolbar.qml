
/* ===
; Copyright (c) 1995-present, Dwyco, Inc.
; 
; This Source Code Form is subject to the terms of the Mozilla Public
; License, v. 2.0. If a copy of the MPL was not distributed with this file,
; You can obtain one at https://mozilla.org/MPL/2.0/.
*/

import QtQuick 2.6
import dwyco 1.0
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.1

ToolBar {
    property Component extras
    background: Rectangle {
        color: accent
    }

    implicitWidth: parent.width


    RowLayout {


        Item {
            Layout.minimumHeight: cm(1)
        }
        anchors.fill: parent
        spacing: mm(2)

        ToolButton {
            id: back_button
            contentItem: Image {
                source: mi("ic_arrow_back_black_24dp.png")
                anchors.centerIn: parent
            }
            checkable: false
            onClicked: {
                stack.pop()
            }
            Layout.fillHeight: true

        }


        Item {

            Layout.fillWidth: true
        }

        ConvToolButton {


        }

        Loader {
            id: extras_loader
            sourceComponent: extras
        }




        //                ToolButton {
        //                    label: Image {
        //                        anchors.centerIn: parent
        //                        source: mi("ic_action_overflow.png")
        //                    }
        //                    onClicked: optionsMenu.open()
        //                    visible: chatbox.visible

        //                    Menu {

        //                        id: optionsMenu
        //                        x: parent.width - width
        //                        transformOrigin: Menu.TopRight

        //                        MenuItem {
        //                            text: "View profile"
        //                            onTriggered: {
        //                                stack.push(theprofileview)
        //                            }
        //                        }

        //                        MenuItem {
        //                            text: "Clear msgs"
        //                            onTriggered: {
        //                                core.clear_messages(chatbox.to_uid)
        //                                themsglist.reload_model()
        //                            }
        //                        }

        //                        MenuItem {
        //                            text: "Delete user"
        //                            onTriggered: {
        //                                core.delete_user(chatbox.to_uid)
        //                                themsglist.reload_model()
        //                                stack.pop()
        //                            }
        //                        }
        //                        MenuItem {
        //                            text: "More..."
        //                            onTriggered: {
        //                                moremenu.open()

        //                            }
        //                        }

        //                    }
        //                }
    }





}
