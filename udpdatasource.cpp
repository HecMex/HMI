#include "udpdatasource.h"
#include "configmanager.h"
#include <QNetworkDatagram>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>

// Definición de constantes estáticas para el canal de retorno hacia el simulador de Python
const QHostAddress UdpDataSource::pythonAddress = QHostAddress("127.0.0.1");
const int UdpDataSource::pythonPort = 5006;

/**
 * @brief Inicializa el socket de red UDP y abre el puerto de escucha.
 *
 * Enlaza la señal nativa de red del sistema operativo con la rutina de lectura interna
 * y realiza el enlace lógico (`bind`) utilizando la dirección IP y puerto parametrizados
 * dinámicamente desde el archivo externo `config.json`.
 *
 * @return true si el socket se enlazó con éxito al puerto de la red local; false en caso de conflicto.
 */
bool UdpDataSource::initialize()
{
    // Instanciar el componente de socket UDP asignando 'this' para gestión automática de memoria
    udpSocket = new QUdpSocket(this);

    // SOLUCIÓN DE CICLO DE VIDA: Conectar de forma nativa la señal de red para lectura asíncrona por eventos
    connect(udpSocket, &QUdpSocket::readyRead, this, &UdpDataSource::readPendingDatagrams);

    // Configurar la interfaz de red local adoptando las variables leídas desde el ConfigManager
    bool success = udpSocket->bind(QHostAddress(ConfigManager::udpAddress), ConfigManager::udpPort);
    if (!success) {
        emit connectionError("Error enlazando puerto UDP: " + QString::number(ConfigManager::udpPort));
    }

    return success;
}

/**
 * @brief Slot asíncrono encargado de consumir y deserializar las ráfagas de paquetes UDP.
 *
 * Se despierta mediante interrupciones lógicas de Qt en cada ráfaga de red. Extrae el texto
 * crudo del datagrama, procesa la sintaxis estructural del documento JSON, valida las llaves
 * de identificación y despacha de forma segura la telemetría procesada.
 */
void UdpDataSource::readPendingDatagrams() {
    // Vaciar secuencialmente el búfer acumulado en la tarjeta de red de la computadora
    while (udpSocket->hasPendingDatagrams()) {
        QNetworkDatagram datagram = udpSocket->receiveDatagram();

        // Convertir el arreglo de bytes binarios (UTF-8) en un documento JSON manipulable
        QJsonDocument doc = QJsonDocument::fromJson(datagram.data());
        if (!doc.isNull() && doc.isObject()) {
            QJsonObject obj = doc.object();

            // Deserialización asociativa de datos (Formato esperado: {"id": "sensor_name", "val": 12.3})
            QString sensorId = obj["id"].toString();

            // SOLUCIÓN DE SEGURIDAD: toVariant().toDouble() blindada
            // Convierte de forma elástica tanto números enteros crudos (ej: 90) como flotantes (90.2)
            // previniendo pérdidas de datos o desbordamientos lógicos si Python omite los decimales.
            double value = obj["val"].toVariant().toDouble();

            // Validar que el identificador del sensor no viaje vacío por la red
            if (!sensorId.isEmpty()) {
                // Notificar de forma unificada (polimórfica) hacia el ExcavatorController
                emit sensorDataReceived(sensorId, value);
            }
        }
    }
}

/**
 * @brief Transmite un datagrama UDP de retorno hacia Python para forzar el paro completo del motor.
 *
 * Actúa como la contraparte digital del comando de bus CAN real, empaquetando la orden
 * de detención en una cadena de texto plana orientada a sockets.
 */
void UdpDataSource::sendStopAllCommand() {
    if (udpSocket) {
        QByteArray command = "STOP_ALL";
        udpSocket->writeDatagram(command, pythonAddress, pythonPort);
        qInfo() << "🚨 HMI envió comando: PARO TOTAL DE MAQUINARIA";
    }
}

/**
 * @brief Transmite un datagrama UDP de retorno hacia Python para bloquear las válvulas de la pala.
 *
 * Mecanismo reactivo para mitigar el peligro de impacto simulado contra la estructura de la cabina.
 */
void UdpDataSource::sendStopShovelCommand() {
    if (udpSocket) {
        QByteArray command = "STOP_SHOVEL";
        udpSocket->writeDatagram(command, pythonAddress, pythonPort);
        qInfo() << "🚨 HMI envió comando: BLOQUEO DE PALA HIDRÁULICA";
    }
}
