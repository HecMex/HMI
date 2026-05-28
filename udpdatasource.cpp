#include "udpdatasource.h"
#include "configmanager.h"
#include <QNetworkDatagram>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>

const QHostAddress UdpDataSource::pythonAddress = QHostAddress("127.0.0.1");
const int UdpDataSource::pythonPort = 5006;

bool UdpDataSource::initialize()
{
    udpSocket = new QUdpSocket(this);

    // Conectar de forma nativa la señal de red
    connect(udpSocket, &QUdpSocket::readyRead, this, &UdpDataSource::readPendingDatagrams);

    // Usa la dirección y puerto mapeados desde el JSON
    bool success = udpSocket->bind(QHostAddress(ConfigManager::udpAddress), ConfigManager::udpPort);
    if (!success) {
        emit connectionError("Error enlazando puerto UDP: " + QString::number(ConfigManager::udpPort));
    }

    return success;
}

void UdpDataSource::readPendingDatagrams() {
    while (udpSocket->hasPendingDatagrams()) {
        QNetworkDatagram datagram = udpSocket->receiveDatagram();

        // Leer el paquete como texto JSON dinámico
        QJsonDocument doc = QJsonDocument::fromJson(datagram.data());
        if (!doc.isNull() && doc.isObject()) {
            QJsonObject obj = doc.object();

            // Extracción de datos del JSON
            QString sensorId = obj["id"].toString();

            // SOLUCCIÓN DE SEGURIDAD: toVariant().toDouble() acepta tanto enteros (90) como flotantes (90.2) sin perder datos
            double value = obj["val"].toVariant().toDouble();

            if (!sensorId.isEmpty()) {
                // SOLUCIÓN: Emitimos la señal genérica idéntica a como la espera el ExcavatorController
                emit sensorDataReceived(sensorId, value);
            }
        }
    }
}

void UdpDataSource::sendStopAllCommand() {
    if (udpSocket) {
        QByteArray command = "STOP_ALL";
        udpSocket->writeDatagram(command, pythonAddress, pythonPort);
        qInfo() << "🚨 HMI envió comando: PARO TOTAL DE MAQUINARIA";
    }
}

void UdpDataSource::sendStopShovelCommand() {
    if (udpSocket) {
        QByteArray command = "STOP_SHOVEL";
        udpSocket->writeDatagram(command, pythonAddress, pythonPort);
        qInfo() << "🚨 HMI envió comando: BLOQUEO DE PALA HIDRÁULICA";
    }
}
