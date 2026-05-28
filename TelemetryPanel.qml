import QtQuick
import QtQuick.Controls

Item {
    id: root
    width: parent.width
    height: 120

    Flow {
        id: dynamicFlow
        anchors.fill: parent
        anchors.margins: 10
        spacing: 20
        // Alinea automáticamente las tarjetas si agregas más sensores
        flow: Flow.LeftToRight

        Repeater {
            // Convierte las llaves del mapa de C++ en el modelo de la lista
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

                    // Muestra el nombre del sensor (la llave del mapa)
                    Text {
                        text: modelData.toUpperCase().replace("_", " ")
                        color: "#BDC3C7"
                        font.bold: true
                        font.pointSize: 10
                        anchors.horizontalCenter: parent.horizontalCenter
                    }

                    // Extrae el valor dinámico en tiempo real de C++
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
