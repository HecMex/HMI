import QtQuick
import QtQuick.Controls

/**
 * @component ExcavatorVisualizer
 * @brief Monitor e instrumental gráfico articulado 2D de la maquinaria pesada.
 *
 * Renderiza de forma geométrica y elástica la inclinación del chasis y calcula
 * la rotación angular del brazo hidráulico para simular el acercamiento a la cabina.
 */
Item {
    id: root
    width: 450
    height: 350

    Rectangle {
        id: excavatorContainer
        anchors.fill: parent
        color: "transparent"

        // Rotación del cuerpo completo basada en el ángulo físico del terreno (C++)
        rotation: excavatorCtrl.inclination
        Behavior on rotation { NumberAnimation { duration: 60; easing.type: Easing.InOutQuad } }

        // --- SUBMÓDULO: ORUGAS (Base estática al suelo) ---
        Rectangle {
            id: tracks
            width: 220
            height: 40
            color: "#2C3E50"
            radius: 12
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 40

            Row {
                anchors.centerIn: parent
                spacing: 15
                Repeater {
                    model: 5
                    Rectangle { width: 24; height: 24; radius: 12; color: "#1A252F" }
                }
            }
        }

        // --- SUBMÓDULO: CABINA PRINCIPAL ---
        Rectangle {
            id: cabinBody
            width: 130
            height: 90
            color: "#F1C40F"
            radius: 6
            anchors.bottom: tracks.top
            anchors.bottomMargin: -5
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.horizontalCenterOffset: -20

            // Parabrisas de la cabina (Punto reactivo de colisión)
            Rectangle {
                width: 45
                height: 55
                // Conmutación cromática inmediata: Cambia a rojo si el brazo viola el límite mínimo seguro
                color: excavatorCtrl.alertCrash ? "#FF3B30" : "#AED6F1"
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
                    visible: excavatorCtrl.alertCrash
                }

                Behavior on color { ColorAnimation { duration: 150 } }
            }
        }

        // --- SUBMÓDULO: BRAZO HIDRÁULICO ARTICULADO ---
        Rectangle {
            id: hydraulicArm
            width: 140
            height: 20
            color: "#D35400"
            radius: 4

            // Punto de anclaje de pivote (Origen de transformación de rotación física)
            transformOrigin: Item.Left

            // Conversión matemática: Mapea la distancia en metros a un desplazamiento angular visual
            rotation: -25 + (excavatorCtrl.armDistance * 20)
            Behavior on rotation { NumberAnimation { duration: 60 } }

            anchors.left: cabinBody.right
            anchors.leftMargin: -10
            anchors.bottom: cabinBody.bottom
            anchors.bottomMargin: 30

            // Cucharón / Pala mecánica
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
