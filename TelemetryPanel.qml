import QtQuick
import QtQuick.Controls

/**
 * @component TelemetryPanel
 * @brief Panel responsivo encargado de renderizar las tarjetas de telemetría digital.
 *
 * Lee dinámicamente el mapa asociativo de C++ (`excavatorCtrl.sensors`) y genera
 * de manera automática una tarjeta visual de estado para cada sensor presente en la red.
 */
Item {
    id: root
    width: parent.width
    height: 120

    /**
     * @brief Contenedor con distribución de flujo elástico.
     * Organiza y envuelve las tarjetas de izquierda a derecha de forma automática.
     */
    Flow {
        id: dynamicFlow
        anchors.fill: parent
        anchors.margins: 10
        spacing: 20
        flow: Flow.LeftToRight

        Repeater {
            // Convierte los nombres de los sensores de C++ en el arreglo del modelo
            model: Object.keys(excavatorCtrl.sensors)

            delegate: Rectangle {
                width: 160
                height: 70
                color: "#2C3E50"
                radius: 6
                border.color: "#34495E"

                Column {
                    anchors.centerIn: parent
                    spacing: 5

                    // Muestra el ID del sensor normalizado a texto legible
                    Text {
                        text: modelData.toUpperCase().replace("_", " ")
                        color: "#BDC3C7"
                        font.bold: true
                        font.pointSize: 10
                        anchors.horizontalCenter: parent.horizontalCenter
                    }

                    // Enlace en tiempo real al valor numérico flotante en C++
                    Text {
                        text: excavatorCtrl.sensors[modelData].toFixed(2)
                        color: "white"
                        font.bold: true
                        font.pointSize: 16
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                }
            }
        }
    }
}
