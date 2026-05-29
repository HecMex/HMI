import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

/**
 * @component Main
 * @brief Ventana raíz y orquestadora principal de la interfaz HMI.
 *
 * Define las dimensiones base de la consola de operación, administra la barra
 * de pestañas de navegación superior y distribuye espacialmente los submódulos
 * de telemetría, gráficos e historial persistente recopilados desde C++.
 */
ApplicationWindow {
    width: 900
    height: 650
    visible: true
    title: "Consola HMI de Excavadora Industrial"
    color: "#1E1E1E"

    /**
     * @brief Control de pestañas principal.
     * Conmuta el índice activo del StackLayout para segmentar las vistas de planta.
     */
    TabBar {
        id: hmiTabBar
        width: parent.width
        currentIndex: 0

        background: Rectangle { color: "#2D2D2D" }

        // Pestaña orientada al control físico y visualización de la excavadora
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

        // Pestaña orientada a la auditoría estática de fallas guardadas en SQLite
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

    /**
     * @brief Gestor asíncrono elástico de pantallas modulares.
     * Sincroniza su propiedad 'currentIndex' de manera reactiva con el TabBar superior.
     */
    StackLayout {
        width: parent.width
        anchors.top: hmiTabBar.bottom
        anchors.bottom: parent.bottom
        currentIndex: hmiTabBar.currentIndex

        // PÁGINA 1: ENTORNO GRÁFICO DE OPERACIÓN EN VIVO
        Item {
            id: operationPage

            // Inserción modular del renderizador geométrico articulado de la máquina
            ExcavatorVisualizer {
                id: excavatorGraphic
                anchors.left: parent.left
                anchors.leftMargin: 40
                anchors.verticalCenter: parent.verticalCenter
            }

            // Inserción modular del panel elástico de alertas rojas/naranjas de seguridad
            AlertPanel {
                id: alertSystem
                anchors.top: parent.top
                anchors.right: parent.right
                anchors.margins: 20
            }

            // Inserción modular de los bloques numéricos adaptativos (Data-Driven)
            TelemetryPanel {
                id: realTimeData
                anchors.bottom: parent.bottom
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.bottomMargin: 20
            }
        }

        // PÁGINA 2: CUADRÍCULA DE AUDITORÍA HISTÓRICA PERMANENTE
        Item {
            id: logHistoryPage

            Rectangle {
                anchors.fill: parent
                anchors.margins: 20
                color: "#2D2D2D"
                border.color: "#444444"
                radius: 6

                // ENCABEZADO ESTÁTICO DE LA TABLA
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

                        Text { text: "Fecha / Hora"; color: "#AAAAAA"; width: 180; font.bold: true; anchors.verticalCenter: parent.verticalCenter }
                        Text { text: "Tipo de Alarma"; color: "#AAAAAA"; width: 180; font.bold: true; anchors.verticalCenter: parent.verticalCenter }
                        Text { text: "Estado"; color: "#AAAAAA"; width: 130; font.bold: true; anchors.verticalCenter: parent.verticalCenter }
                        Text { text: "Detalles Técnicos"; color: "#AAAAAA"; width: 250; font.bold: true; anchors.verticalCenter: parent.verticalCenter }
                    }
                }

                // LISTA DINÁMICA SCROLLABLE DEL HISTORIAL SQLITE
                ListView {
                    anchors.top: tableHeader.bottom
                    anchors.bottom: parent.bottom
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.margins: 5
                    clip: true

                    // Vinculación directa con el QVariantList expuesto desde C++
                    model: excavatorCtrl.alarmHistory

                    // Delegado visual encargado de pintar cada fila indexada
                    delegate: Rectangle {
                        width: parent.width
                        height: 38
                        color: index % 2 === 0 ? "#252525" : "#2D2D2D" // Color de fila intercalado para legibilidad

                        Row {
                            anchors.fill: parent
                            anchors.leftMargin: 10
                            spacing: 10

                            // Extraer propiedades del mapa de C++ mapeándolas de forma elástica en Javascript
                            Text { text: modelData.timestamp || ""; color: "white"; width: 180; anchors.verticalCenter: parent.verticalCenter }
                            Text { text: modelData.type || ""; color: "white"; width: 180; anchors.verticalCenter: parent.verticalCenter }

                            // Conmutador cromático industrial basado en la severidad del estado de la alarma
                            Text {
                                text: modelData.state || ""
                                width: 130
                                font.bold: true
                                anchors.verticalCenter: parent.verticalCenter
                                color: modelData.state === "ACTIVADA" ? "#FF3B30" : // Rojo peligro
                                       (modelData.state === "RECONOCIDA" ? "#5856D6" : "#34C759") // Púrpura ACK o Verde Seguro
                            }

                            Text { text: modelData.details || ""; color: "#D1D1D6"; width: 250; anchors.verticalCenter: parent.verticalCenter }
                        }
                    }
                    // Barra de desplazamiento vertical nativa táctil/mouse
                    ScrollBar.vertical: ScrollBar { active: true }
                }
            }
        }
    }

    /**
     * @brief Bloqueo de seguridad tras pérdida de ráfagas de telemetría.
     *
     * Se activa por encima de cualquier otra pestaña (z: 999) interceptando
     * eventos físicos del operador para impedir mandos inválidos si la máquina
     * pierde comunicación por cable CAN o socket de red local UDP.
     */
    Rectangle {
        id: commLossOverlay
        anchors.fill: parent
        visible: excavatorCtrl.commLoss // Enlazado a la propiedad 'm_commLoss' administrada por el QTimer de C++
        color: "#CC000000" // Opacidad oscura al 80% para enmascarar la HMI de fondo
        z: 999             // Coordenada z máxima: Bloquea accesos al TabBar superior

        // Interceptor de hardware: Secuestra clics y toques para blindar los botones de fondo
        MouseArea {
            anchors.fill: parent
            preventStealing: true
        }

        Column {
            anchors.centerIn: parent
            spacing: 20

            // Icono de advertencia volumétrico animado con parpadeo infinito
            Text {
                text: "⚠️"
                font.pointSize: 60
                anchors.horizontalCenter: parent.horizontalCenter

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
