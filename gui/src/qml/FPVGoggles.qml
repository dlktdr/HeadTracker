import QtQuick 2.15
import QtQuick3D 1.15

Node {
    id: rootNode

    Model {
        id: pasted__pCylinder3
        y: -165
        z: -27
        eulerRotation.x: -90
        source: "meshes/pasted__pCylinder3.mesh"

        DefaultMaterial {
            id: lambert6_material
            diffuseColor: "#ff4a3200"
        }

        DefaultMaterial {
            id: lambert4_material
            diffuseColor: "#ff020202"
        }

        DefaultMaterial {
            id: lambert5_material
            diffuseColor: "#ff550000"
        }

        DefaultMaterial {
            id: lambert3_material
            diffuseColor: "#ff272727"
        }

        DefaultMaterial {
            id: lambert2_material
            diffuseColor: "#ff040404"
        }

        DefaultMaterial {
            id: blinn1_material
            diffuseColor: "#ff01232e"
        }
        materials: [
            lambert6_material,
            lambert4_material,
            lambert5_material,
            lambert3_material,
            lambert2_material,
            blinn1_material
        ]
    }

    PointLight {
        id: area
        y: 922
        z: -515
        eulerRotation.x: 64.6
        color: "#ffffffff"
    }

    PerspectiveCamera {
        id: camera
        y: 1106
        eulerRotation.x: 81.4758
        eulerRotation.y: -4.46988
        eulerRotation.z: 4.92418e-06
        clipFar: 100000
        fieldOfView: 39.5978
        fieldOfViewOrientation: Camera.Horizontal
    }

    SpotLight {
        id: spot
        x: -44
        z: 1395
        eulerRotation.x: -176.066
        eulerRotation.y: 0.000493947
        eulerRotation.z: -89.9948
        color: "#ffffffff"
        quadraticFade: 3.2e-07
        coneAngle: 56.8
        innerConeAngle: 48.28
    }

    SpotLight {
        id: spot_001
        y: -920
        z: 445
        eulerRotation.x: -123.4
        color: "#ffffffff"
        quadraticFade: 3.2e-07
        coneAngle: 90.3
        innerConeAngle: 76.755
    }
}
