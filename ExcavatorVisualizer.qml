import QtQuick
import QtQuick.Controls

Item {
    id: root
    width: 450
    height: 350

    // CONTENEDOR PRINCIPAL DE LA EXCAVADORA
    Rectangle {
        id: excavatorContainer
        anchors.fill: parent
        color: "transparent"

        // Rotación animada y fluida basada en el controlador de C++
        rotation: excavatorCtrl.inclination
        Behavior on rotation { NumberAnimation { duration: 60; easing.type: Easing.InOutQuad } }

        // --- SUBMÓDULO: ORUGAS / LLANTAS (Base fija al suelo) ---
        Rectangle {
            id: tracks
            width: 220
            height: 40
            color: "#2C3E50" // Gris oscuro mecánico
            radius: 12
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 40

            // Detalle estético de los rodajes internos
            Row {
                anchors.centerIn: parent
                spacing: 15
                Repeater {
                    model: 5
                    Rectangle { width: 24; height: 24; radius: 12; color: "#1A252F" }
                }
            }
        }

        // --- SUBMÓDULO: CABINA Y CUERPO PRINCIPAL ---
        Rectangle {
            id: cabinBody
            width: 130
            height: 90
            color: "#F1C40F" // Amarillo industrial / CAT
            radius: 6
            anchors.bottom: tracks.top
            anchors.bottomMargin: -5
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.horizontalCenterOffset: -20 // Desplazado para hacer espacio al brazo

            // Parabrisas de la cabina (Punto de referencia visual para la alerta de proximidad)
            Rectangle {
                width: 45
                height: 55
                color: excavatorCtrl.alertCrash ? "#FF3B30" : "#AED6F1" // Se ilumina en rojo si el brazo se acerca
                radius: 4
                anchors.top: parent.top
                anchors.topMargin: 10
                anchors.right: parent.right
                anchors.rightMargin: 10

                Text {
                    text: "⚠"
                    color: "white"
                    font.bold: true
                    font.pointSize: 14
                    anchors.centerIn: parent
                    visible: excavatorCtrl.alertCrash // Solo parpadea el icono de peligro si hay riesgo de choque
                }

                Behavior on color { ColorAnimation { duration: 150 } }
            }
        }

        // --- SUBMÓDULO: BRAZO HIDRÁULICO ARTICULADO (Efector final) ---
        Rectangle {
            id: hydraulicArm
            width: 140
            height: 20
            color: "#D35400" // Naranja mecánico
            radius: 4

            // Punto de anclaje cinemático: el brazo rota respecto a la unión con la cabina
            transformOrigin: Item.Left

            // Mapeo inverso de la distancia enviada por Python para simular la aproximación física
            // Si la distancia disminuye en el sensor, el brazo rota visualmente cerrándose hacia la cabina
            rotation: -25 + (excavatorCtrl.armDistance * 20)
            Behavior on rotation { NumberAnimation { duration: 60 } }

            anchors.left: cabinBody.right
            anchors.leftMargin: -10
            anchors.bottom: cabinBody.bottom
            anchors.bottomMargin: 30

            // Pala / Cucharón en la punta del brazo
            Rectangle {
                width: 45
                height: 45
                color: "#7F8C8D"
                radius: 4
                rotation: 35
                anchors.right: parent.right
                anchors.rightMargin: -15
                anchors.verticalCenter: parent.verticalCenter
            }
        }
    }
}
