import QtQuick 2.15
import QtQuick.Window 2.14
import QtQuick3D 1.15
import QtQuick.Controls 2.15
import Qt.example.qobjectSingleton 1.0

Rectangle {
  id: window
  width: 640
  height: 480
  visible: true
  color: "#f0f0f0"

  function getAngle() {
      if(!compassrawmag.checked) {
          return -1.0 * TrackerSettings.liveData["panoff"]
      } else {
          return Math.atan2(TrackerSettings.liveData["off_magy"],TrackerSettings.liveData["off_magx"]) * 180 / Math.PI
      }
  }

  Rectangle {
    id: compass
    anchors.top: parent.top
    anchors.right: parent.right
    width: 200
    height: 200
    color: "transparent"

     Image {
        id: compass_back
        width: parent.width
        height: parent.height
        source: "compass_back.svg"
     }

     Image {
         id: compass_needle
         width: parent.width
         height: parent.height
         source: "compass_needle.svg"
         rotation: getAngle()
     }
  }

  Text {
      id: text
      anchors.top: parent.top
      anchors.left: parent.left
      anchors.margins: 5

      color: "black"
      font.pixelSize: 17
      font.bold: true
      text: qsTr("HeadTracker 3D")
  }

  CheckBox {
      id: compassrawmag
      anchors.top: text.bottom
      anchors.left: parent.left
      text: qsTr("Compass=Atan2(x,y)")
      checked: false
  }

  View3D {
      id: view
      anchors.fill: parent
      camera: camera
      renderMode: View3D.Overlay

      PerspectiveCamera {
          id: camera
          position: Qt.vector3d(0, 0, 300)
          eulerRotation.x: 0
      }

        DirectionalLight {
            ambientColor: Qt.rgba(0.5,0.5,0.5,1.0)
            brightness: 3000
            position: Qt.vector3d(0,800,800)
            eulerRotation.x: -45
            eulerRotation.y: 0
            eulerRotation.z: 0
        }

      FPVGoggles {
        id: nano
        visible: true
        position: Qt.vector3d(-0, -80, 0)
        scale: Qt.vector3d(0.2,0.2,0.2)
        eulerRotation.x: TrackerSettings.liveData["tiltoff"]
        eulerRotation.y: TrackerSettings.liveData["panoff"]
        eulerRotation.z: -1.0 * TrackerSettings.liveData["rolloff"]


      }
  }
}

  /*MouseArea {
      id: mouseArea
      anchors.fill: qt_logo

      Text {
          id: clickme
          anchors.top: mouseArea.top
          anchors.horizontalCenter: mouseArea.horizontalCenter
          font.pixelSize: 17
          text: "Click me!"
          color: "white"

          SequentialAnimation on color {
              loops: Animation.Infinite
              ColorAnimation { duration: 400; from: "white"; to: "black" }
              ColorAnimation { duration: 400; from: "black"; to: "white" }
          }

          states: [
              State {
                  name: "flipped";
                  AnchorChanges {
                      target: clickme
                      anchors.top: undefined
                      anchors.bottom: mouseArea.bottom
                  }
              }
          ]
      }

      onClicked: {
          if (clickme.state == "flipped") {
              clickme.state = "";
              flip2.start();
          } else {
              clickme.state = "flipped";
              flip1.start();
          }
      }
  }*/
