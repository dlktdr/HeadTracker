import QtQuick 2.15
import QtQuick3D 1.15

Node {
    id: rootNode

    PerspectiveCamera {
        id: camera
        x: 735.889
        y: 495.831
        z: 692.579
        eulerRotation.x: -153.559
        eulerRotation.y: -46.6919
        eulerRotation.z: -180
        fieldOfView: 39.5978
        fieldOfViewOrientation: Camera.Horizontal
    }

    Node {
        id: nano33BLE_v1
        scale.x: 20
        scale.y: 20
        scale.z: 20

        Node {
            id: nANO33BLE_V2_0_v1_1

            Node {
                id: nANO33BLE_V2_0_v1

                Node {
                    id: board__1__1

                    Node {
                        id: board__1_

                        Model {
                            id: board
                            scale.x: 10
                            scale.y: 10
                            scale.z: 10
                            source: "meshes/board.mesh"

                            DefaultMaterial {
                                id: opaque_0_128_0__material
                                diffuseColor: "#ff264b80"
                            }

                            DefaultMaterial {
                                id: opaque_150_150_150__material
                                diffuseColor: "#ff969696"
                            }

                            DefaultMaterial {
                                id: opaque_50_50_50__material
                                diffuseColor: "#ff323232"
                            }

                            DefaultMaterial {
                                id: opaque_185_185_185__material
                                diffuseColor: "#ffb9b9b9"
                            }

                            DefaultMaterial {
                                id: aluminum___Satin_material
                                diffuseColor: "#fff5f5f6"
                            }

                            DefaultMaterial {
                                id: opaque_229_229_229__material
                                diffuseColor: "#ffe5e5e5"
                            }

                            DefaultMaterial {
                                id: opaque_255_241_177__material
                                diffuseColor: "#fffff1b1"
                            }

                            DefaultMaterial {
                                id: opaque_244_244_244__material
                                diffuseColor: "#fff4f4f4"
                            }

                            DefaultMaterial {
                                id: opaque_111_111_111__material
                                diffuseColor: "#ff6f6f6f"
                            }

                            DefaultMaterial {
                                id: opaque_245_245_245__material
                                diffuseColor: "#fff5f5f5"
                            }

                            DefaultMaterial {
                                id: opaque_211_214_211__material
                                diffuseColor: "#ffd3d6d3"
                            }

                            DefaultMaterial {
                                id: opaque_64_64_64__material
                                diffuseColor: "#ff404040"
                            }

                            DefaultMaterial {
                                id: opaque_124_60_4__material
                                diffuseColor: "#ff7c3c04"
                            }

                            DefaultMaterial {
                                id: opaque_255_255_255__material
                            }

                            DefaultMaterial {
                                id: opaque_202_209_238__material
                                diffuseColor: "#ffcad1ee"
                            }

                            DefaultMaterial {
                                id: opaque_255_207_128__material
                                diffuseColor: "#ffffcf80"
                            }

                            DefaultMaterial {
                                id: opaque_128_128_128__material
                                diffuseColor: "#ff808080"
                            }
                            materials: [
                                opaque_0_128_0__material,
                                opaque_150_150_150__material,
                                opaque_50_50_50__material,
                                opaque_185_185_185__material,
                                aluminum___Satin_material,
                                opaque_229_229_229__material,
                                opaque_255_241_177__material,
                                opaque_244_244_244__material,
                                opaque_111_111_111__material,
                                opaque_245_245_245__material,
                                opaque_211_214_211__material,
                                opaque_64_64_64__material,
                                opaque_124_60_4__material,
                                opaque_255_255_255__material,
                                opaque_202_209_238__material,
                                opaque_255_207_128__material,
                                opaque_128_128_128__material
                            ]
                        }
                    }
                }

            }
        }
    }
}
