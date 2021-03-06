
/* ===
; Copyright (c) 1995-present, Dwyco, Inc.
; 
; This Source Code Form is subject to the terms of the Mozilla Public
; License, v. 2.0. If a copy of the MPL was not distributed with this file,
; You can obtain one at https://mozilla.org/MPL/2.0/.
*/
import QtQuick 2.9
import QtQuick.Controls 2.2
import QtQuick.Controls.Material 2.2
import dwyco 1.0
import QtQuick.Layouts 1.3
import QtMultimedia 5.6

Page {
    property bool dragging

    anchors.fill: parent

    header: SimpleToolbar {
        //extras: extras_button

    }

    onVisibleChanged: {
        if(visible)
        {
            core.enable_video_capture_preview(1)
            //preview_cam.stop()
        }
        else
            core.enable_video_capture_preview(0)

    }

    Camera {
        id: preview_cam
        objectName: "qrCameraQML"
        viewfinder {
            resolution: Qt.size(640, 480)
            //maximumFrameRate: 10
        }
        position: Camera.FrontFace
        captureMode: Camera.captureVideo
        onCameraStateChanged: {
            //if(state === Camera.ActiveState) {
                var res = preview_cam.supportedViewfinderResolutions();
                console.log("RESOLUTIONS ")
            for(var i = 0; i < res.length; i++) {
                console.log(res[i].width)
                console.log(res[i].height)
            }
            //}
        }
        onCameraStatusChanged: {
            //if(state === Camera.ActiveState) {
                var res = preview_cam.supportedViewfinderResolutions();
                console.log("RESOLUTIONS ")
            for(var i = 0; i < res.length; i++) {
                console.log(res[i].width)
                console.log(res[i].height)
            }

            //}
        }
    }
    VideoOutput {
        id: pview
        width: cm(1)
        height: cm(1)
        source: preview_cam
        anchors.top: parent.top
        anchors.horizontalCenter: parent.horizontalCenter
        autoOrientation: true
        visible: true

    }


    ListView {
        //Layout.fillWidth: true
        anchors.top: pview.bottom
        //anchors.top: parent.top
        anchors.left: parent.left
        anchors.right:parent.right
        height: cm(3)
        id: camlist
        model: camListModel
        delegate: camlist_delegate
        clip: true
        spacing: 5
    }

    Component {
        id: camlist_delegate
        Rectangle {
            height: mm(4)
            width: camlist.width
            Label {
                id: name
                anchors.fill: parent
                text: modelData
                verticalAlignment: Text.verticalCenter
            }
            MouseArea {
                anchors.fill: parent
                onClicked: {
//                    if(index === 0)
//                        preview_cam.stop()
                    camlist.currentIndex = index
                    core.select_vid_dev(index)
                }
            }
        }
    }

    Rectangle {
        anchors.top: camlist.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        //Layout.fillWidth: true
        //Layout.fillHeight: true

        Image {
            id: viewer
            anchors.top: dragging ? undefined : parent.top
            anchors.right: dragging ? undefined : parent.right
            anchors.left: dragging ? undefined : parent.left
            anchors.bottom: dragging ? undefined : parent.bottom
            //anchors.horizontalCenter: parent.horizontalCenter
            fillMode: Image.PreserveAspectFit
            onVisibleChanged: {
                dragging = false
                scale = 1.0
            }
            Connections {
                target: core
                onVideo_capture_preview: {
                    if(visible)
                        viewer.source = img_path
                }

            }

        }

        PinchArea {
            anchors.fill: parent
            pinch.target: viewer
            pinch.minimumScale: 0.1
            pinch.maximumScale: 10
            pinch.dragAxis: Pinch.XAndYAxis

            MouseArea {
                id: dragArea
                hoverEnabled: true
                anchors.fill: parent
                drag.target: viewer
                scrollGestureEnabled: false

                onPressed: {
                    dragging = true

                }
                onClicked: {
                    stack.pop()

                }
            }
        }
    }

    BusyIndicator {
        id: busy1
        running: {viewer.source == "" }
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.verticalCenter: parent.verticalCenter
    }

}
