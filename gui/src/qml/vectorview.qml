import QtQuick 2.15
import QtQuick.Window 2.14
import QtQuick3D 1.15
import Qt.example.qobjectSingleton 1.0

Rectangle {
  id: window
  width: 640
  height: 640
  visible: true
  color: "black"

  Rectangle {
      id: qt_logo
      width: 230
      height: 230
      anchors.top: parent.top
      anchors.left: parent.left
      anchors.margins: 10

      color: "transparent"

      layer.enabled: true

      Rectangle {
          anchors.fill: parent
          color: "black"

          Text {
              id: text
              anchors.top: parent.top
              anchors.left: parent.left
              //anchors.topMargin: 100

              color: "white"
              font.pixelSize: 17
              text: qsTr("HEADTRACKER 3D!")
              //rotation: TrackerSettings.liveData["tiltoff"]
          }
      }
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
          eulerRotation.x: 0
      }

      Nano33BLE {
        id: nano
        visible: true
        position: Qt.vector3d(-125, 0, 0)
        scale: Qt.vector3d(0.3,0.3,0.3)
        eulerRotation.x: (-1.0 *TrackerSettings.liveData["tiltoff"]) - 90
        eulerRotation.z: TrackerSettings.liveData["rolloff"]
        eulerRotation.y: TrackerSettings.liveData["panoff"]
      }

      /*Model {
          id: cube
          visible: true
          position: Qt.vector3d(0, 0, 0)
          source: "#Cube"
          materials: [ DefaultMaterial {
                  diffuseMap: Texture {
                      id: texture
                      sourceItem: qt_logo
                  }
              }
          ]
          eulerRotation.x: -TrackerSettings.liveData["tiltoff"]
          eulerRotation.z: TrackerSettings.liveData["rolloff"]
          eulerRotation.y: TrackerSettings.liveData["panoff"]


          /*SequentialAnimation on eulerRotation {
              loops: Animation.Infinite
              PropertyAnimation {
                  duration: 5000
                  from: Qt.vector3d(0, 0, 0)
                  to: Qt.vector3d(360, 0, 360)
              }
          }*/
      //}
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
}
