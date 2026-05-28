import QtQuick
import QtQuick.Controls

Column {
    id: root
    spacing: 15
    width: 260 // Mantiene el ancho estático del panel lateral derecho

    Repeater {
        // Genera un bloque visual por cada sensor que exista en el mapa de C++
        model: Object.keys(excavatorCtrl.sensors)

        delegate: Item {
            id: alarmItem
            width: parent ? parent.width : 260

            // Extracción del valor actual desde el mapa dinámico
            property double currentVal: excavatorCtrl.sensors[modelData]

            // Consulta de límites usando la función puente del controlador
            property double maxLimit: excavatorCtrl.getDynamicLimit(modelData + "_max")
            property double minLimit: excavatorCtrl.getDynamicLimit(modelData + "_min")

            // Evaluación de criticidad
            property bool isCriticalMax: maxLimit !== -1.0 && currentVal >= maxLimit
            property bool isCriticalMin: minLimit !== -1.0 && currentVal <= minLimit
            property bool isAlertActive: isCriticalMax || isCriticalMin

            // Textos adaptativos según el ID del sensor
            property string friendlyName: modelData === "inclinacion" ? "INCLINACIÓN" :
                                          modelData === "distancia_brazo" ? "DISTANCIA BRAZO" :
                                          modelData === "temp_motor" ? "TEMP. MOTOR" : modelData.toUpperCase().replace("_", " ")

            property string actionText: modelData === "inclinacion" ? "APAGAR EXCAVADORA" :
                                        modelData === "distancia_brazo" ? "BLOQUEAR MOV. PALA" : "DETENER COMPONENTE"

            // La tarjeta solo es visible y ocupa espacio si la alarma está activa
            visible: isAlertActive
            height: visible ? 130 : 0

            Behavior on height { NumberAnimation { duration: 200 } }
            opacity: visible ? 1.0 : 0.0
            Behavior on opacity { NumberAnimation { duration: 150 } }

            Rectangle {
                anchors.fill: parent
                // Naranja para el brazo (Riesgo de choque local), Rojo para inclinación/motor (Riesgo total de volcadura)
                color: modelData === "distancia_brazo" ? "#FF9500" : "#FF3B30"
                radius: 8
                clip: true

                Column {
                    anchors.fill: parent
                    anchors.margins: 10
                    spacing: 10

                    Text {
                        text: "⚠️ " + alarmItem.friendlyName + " CRÍTICA"
                        color: "white"
                        font.bold: true
                        font.pointSize: 11
                        anchors.horizontalCenter: parent.horizontalCenter
                    }

                    Button {
                        text: alarmItem.actionText
                        width: parent.width
                        height: 45
                        enabled: alarmItem.isAlertActive

                        onClicked: {
                            if (modelData === "inclinacion") {
                                excavatorCtrl.requestEngineShutdown();
                            } else if (modelData === "distancia_brazo") {
                                excavatorCtrl.requestShovelLock();
                            } else {
                                console.log("🚨 HMI: Acción genérica de paro ejecutada para " + modelData);
                            }
                        }

                        background: Rectangle {
                            color: parent.enabled ? "#000000" : "#555555"
                            radius: 4
                        }
                        contentItem: Text {
                            text: parent.text
                            color: parent.enabled ? (modelData === "distancia_brazo" ? "#FF9500" : "#FF3B30") : "#AAAAAA"
                            font.bold: true
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                    }
                }
            }
        }
    }
}
