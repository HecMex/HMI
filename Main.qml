import QtQuick
import QtQuick.Controls
import QtQuick.Layouts // <-- Requerido para StackLayout

ApplicationWindow {
    width: 900
    height: 650
    visible: true
    title: "Consola HMI de Excavadora Industrial"
    color: "#1E1E1E"

    // --- BARRA DE PESTAÑAS SUPERIOR ---
    TabBar {
        id: hmiTabBar
        width: parent.width
        currentIndex: 0

        background: Rectangle { color: "#2D2D2D" }

        TabButton {
            text: "⚙️ OPERACIÓN"
            contentItem: Text {
                text: parent.text
                font.bold: true
                color: parent.checked ? "#F1C40F" : "#AAAAAA"
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
        }
        TabButton {
            text: "📋 HISTORIAL DE ALARMAS"
            contentItem: Text {
                text: parent.text
                font.bold: true
                color: parent.checked ? "#FF3B30" : "#AAAAAA"
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
        }
    }

    // --- CONTENEDOR DE PÁGINAS DINÁMICAS ---
    StackLayout {
        width: parent.width
        anchors.top: hmiTabBar.bottom
        anchors.bottom: parent.bottom
        currentIndex: hmiTabBar.currentIndex

        // ==========================================
        // PÁGINA 1: MONITOR DE EXCAVADORA Y ALERTAS
        // ==========================================
        Item {
            id: operationPage

            // Inserción modular del renderizador 2D de la excavadora
            ExcavatorVisualizer {
                id: excavatorGraphic
                anchors.left: parent.left
                anchors.leftMargin: 40
                anchors.verticalCenter: parent.verticalCenter
            }

            // Inserción modular del panel lateral de alertas críticas
            AlertPanel {
                id: alertSystem
                anchors.top: parent.top
                anchors.right: parent.right
                anchors.margins: 20
            }

            // Inserción modular de los textos de telemetría digital
            TelemetryPanel {
                id: realTimeData
                anchors.bottom: parent.bottom
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.bottomMargin: 20
            }
        }

        // ==========================================
        // PÁGINA 2: AUDITORÍA
        // ==========================================
        Item {
            id: logHistoryPage

            Rectangle {
                anchors.fill: parent
                anchors.margins: 20
                color: "#2D2D2D"
                border.color: "#444444"
                radius: 6

                // Encabezado
                Rectangle {
                    id: tableHeader
                    height: 40
                    color: "#3A3A3C"
                    radius: 6
                    anchors.top: parent.top
                    anchors.left: parent.left
                    anchors.right: parent.right

                    Row {
                        anchors.fill: parent
                        anchors.leftMargin: 15
                        spacing: 10

                        Text {
                            text: "Fecha / Hora"
                            color: "#AAAAAA"
                            width: 180
                            font.bold: true
                            anchors.verticalCenter: parent.verticalCenter
                        }
                        Text { text: "Tipo de Alarma"; color: "#AAAAAA"; width: 180; font.bold: true; anchors.verticalCenter: parent.verticalCenter }
                        Text { text: "Estado"; color: "#AAAAAA"; width: 130; font.bold: true; anchors.verticalCenter: parent.verticalCenter }
                        Text { text: "Detalles Técnicos"; color: "#AAAAAA"; width: 250; font.bold: true; anchors.verticalCenter: parent.verticalCenter }
                    }
                }

                // Lista
                ListView {
                    anchors.top: tableHeader.bottom
                    anchors.bottom: parent.bottom
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.margins: 5
                    clip: true
                    model: excavatorCtrl.alarmHistory

                    delegate: Rectangle {
                        width: parent.width
                        height: 38
                        color: index % 2 === 0 ? "#252525" : "#2D2D2D"

                        Row {
                            anchors.fill: parent
                            anchors.leftMargin: 10
                            spacing: 10

                            Text { text: modelData.timestamp || ""; color: "white"; width: 180; anchors.verticalCenter: parent.verticalCenter }
                            Text { text: modelData.type || ""; color: "white"; width: 180; anchors.verticalCenter: parent.verticalCenter }

                            Text {
                                text: modelData.state || ""
                                width: 130
                                font.bold: true
                                anchors.verticalCenter: parent.verticalCenter
                                color: modelData.state === "ACTIVADA" ? "#FF3B30" :
                                       (modelData.state === "RECONOCIDA" ? "#5856D6" : "#34C759")
                            }

                            Text { text: modelData.details || ""; color: "#D1D1D6"; width: 250; anchors.verticalCenter: parent.verticalCenter }
                        }
                    }
                    ScrollBar.vertical: ScrollBar { active: true }
                }
            }
        }
    }
    // --- CAPA DE BLOQUEO POR PÉRDIDA DE COMUNICACIÓN (WATCHDOG) ---
    Rectangle {
        id: commLossOverlay
        anchors.fill: parent
        // Se vuelve visible y bloquea los clics si se activa la falla en C++
        visible: excavatorCtrl.commLoss
        color: "#CC000000" // Fondo negro translúcido (Opacidad del 80%)
        z: 999             // Asegura que se dibuje por encima de las pestañas y botones

        // Intercepta todos los eventos de mouse para que no se pueda presionar nada de fondo
        MouseArea {
            anchors.fill: parent
            preventStealing: true
        }

        Column {
            anchors.centerIn: parent
            spacing: 20

            // Icono parpadeante de advertencia
            Text {
                text: "⚠️"
                font.pointSize: 60
                anchors.horizontalCenter: parent.horizontalCenter

                // Animación simple de parpadeo continuo
                SequentialAnimation on opacity {
                    loops: Animation.Infinite
                    NumberAnimation { from: 1.0; to: 0.2; duration: 500 }
                    NumberAnimation { from: 0.2; to: 1.0; duration: 500 }
                }
            }

            Text {
                text: "ERROR: PÉRDIDA DE CONEXIÓN CON LA EXCAVADORA"
                color: "#FF3B30"
                font.bold: true
                font.pointSize: 22
                anchors.horizontalCenter: parent.horizontalCenter
            }

            Text {
                text: "Compruebe el cableado del Bus CAN / Red UDP o reinicie el simulador físico."
                color: "white"
                font.pointSize: 14
                anchors.horizontalCenter: parent.horizontalCenter
            }
        }
    }
}
