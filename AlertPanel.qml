import QtQuick
import QtQuick.Controls

/**
 * @component AlertPanel
 * @brief Panel de control lateral derecho autoadaptable para alarmas críticas.
 *
 * Monitorea el mapa de sensores y consulta la función puente de C++ para evaluar límites.
 * Si un sensor entra en estado de peligro, despliega dinámicamente una tarjeta de
 * emergencia y habilita botones interactivos de paro bidireccional.
 */
Column {
    id: root
    spacing: 15
    width: 260

    Repeater {
        // Evalúa cíclicamente cada sensor registrado en el sistema
        model: Object.keys(excavatorCtrl.sensors)

        delegate: Item {
            id: alarmItem
            width: root.width // Mapea el ancho de la columna raíz

            // -------------------------------------------------------------
            // PROPIEDADES DE ANÁLISIS DE CRITICIDAD DE LA ALARMA
            // -------------------------------------------------------------
            /** @property double currentVal Almacena la lectura instantánea del sensor actual */
            property double currentVal: excavatorCtrl.sensors[modelData]

            /** @property double maxLimit Recupera el umbral máximo configurado en el JSON */
            property double maxLimit: excavatorCtrl.getDynamicLimit(modelData + "_max")

            /** @property double minLimit Recupera el umbral mínimo configurado en el JSON */
            property double minLimit: excavatorCtrl.getDynamicLimit(modelData + "_min")

            /** @property bool isCriticalMax Determina si el valor superó el umbral superior */
            property bool isCriticalMax: maxLimit !== -1.0 && currentVal >= maxLimit

            /** @property bool isCriticalMin Determina si el valor cayó por debajo del umbral inferior */
            property bool isCriticalMin: minLimit !== -1.0 && currentVal <= minLimit

            /** @property bool isAlertActive Bandera global que activa visualmente la tarjeta */
            property bool isAlertActive: isCriticalMax || isCriticalMin

            // -------------------------------------------------------------
            // PROPIEDADES ADAPTATIVAS DE TEXTOS Y COMANDOS
            // -------------------------------------------------------------
            property string friendlyName: modelData === "inclinacion" ? "INCLINACIÓN" :
                                          modelData === "distancia_brazo" ? "DISTANCIA BRAZO" :
                                          modelData === "temp_motor" ? "TEMP. MOTOR" : modelData.toUpperCase().replace("_", " ")

            property string actionText: modelData === "inclinacion" ? "APAGAR EXCAVADORA" :
                                        modelData === "distancia_brazo" ? "BLOQUEAR MOV. PALA" : "DETENER COMPONENTE"

            // La tarjeta se colapsa (height: 0) e invisibiliza si el proceso está en rangos seguros
            visible: isAlertActive
            height: visible ? 130 : 0

            // Comportamiento de transiciones fluidas de tamaño y opacidad
            Behavior on height { NumberAnimation { duration: 200 } }
            opacity: visible ? 1.0 : 0.0
            Behavior on opacity { NumberAnimation { duration: 150 } }

            Rectangle {
                anchors.fill: parent
                // Código visual de seguridad: Naranja para advertencia de choque, Rojo para paros totales de motor
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
                            // Enrutar la pulsación del operador hacia los comandos binarios de C++
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
