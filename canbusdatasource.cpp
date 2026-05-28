#include "canbusdatasource.h"
#include "configmanager.h"
#include <QCanBus>
#include <QDataStream>
#include <QDebug>

CanBusDataSource::CanBusDataSource(QObject *parent)
    : ExcavatorDataSource(parent), m_canDevice(nullptr) {}

CanBusDataSource::~CanBusDataSource() {
    CanBusDataSource::disconnectSource();
}

bool CanBusDataSource::initialize() {
    if (!QCanBus::instance()->plugins().contains(ConfigManager::canPlugin)) {
        emit connectionError(QString("Plugin CAN no disponible: %1").arg(ConfigManager::canPlugin));
        return false;
    }

    QString errorStr;
    m_canDevice = QCanBus::instance()->createDevice(ConfigManager::canPlugin, ConfigManager::canInterface, &errorStr);

    if (!m_canDevice) {
        emit connectionError("Error al crear dispositivo CAN: " + errorStr);
        return false;
    }

    connect(m_canDevice, &QCanBusDevice::framesReceived, this, &CanBusDataSource::readCanFrames);
    connect(m_canDevice, &QCanBusDevice::errorOccurred, this, &CanBusDataSource::handleDeviceError);

    bool success = m_canDevice->connectDevice();
    if (success) {
        qInfo() << "✅ Conectado con éxito al Bus CAN Industrial:" << ConfigManager::canInterface;
    } else {
        emit connectionError("Fallo al conectar al bus CAN físico: " + m_canDevice->errorString());
    }
    return success;
}

void CanBusDataSource::disconnectSource() {
    if (m_canDevice) {
        if (m_canDevice->state() == QCanBusDevice::ConnectedState) {
            m_canDevice->disconnectDevice();
        }
        delete m_canDevice;
        m_canDevice = nullptr;
        qInfo() << "🔌 Bus CAN desconectado de forma segura.";
    }
}

void CanBusDataSource::readCanFrames() {
    if (!m_canDevice) return;

    while (m_canDevice->framesAvailable()) {
        const QCanBusFrame frame = m_canDevice->readFrame();

        // 1. Convertir el ID numérico de la trama a formato String Hex ("0x1CFDD600")
        QString frameIdStr = QString("0x%1").arg(frame.frameId(), 8, 16, QChar('0')).toUpper();

        // 2. Verificar de forma dinámica si este ID de CAN está registrado en nuestro JSON
        if (ConfigManager::canMapping.contains(frameIdStr)) {
            QString sensorId = ConfigManager::canMapping.value(frameIdStr);
            QByteArray payload = frame.payload();

            // 3. Desempaquetar el valor numérico de los 8 bytes de la trama CAN
            if (payload.size() >= sizeof(double)) {
                QDataStream stream(payload);
                stream.setByteOrder(QDataStream::LittleEndian);
                stream.setFloatingPointPrecision(QDataStream::DoublePrecision);

                double value = 0.0;
                stream >> value;

                // 4. Emitir la señal polimórfica genérica que procesa el ExcavatorController
                emit sensorDataReceived(sensorId, value);
            }
        }
    }
}
void CanBusDataSource::sendStopAllCommand()
{
    if (!m_canDevice || m_canDevice->state() != QCanBusDevice::ConnectedState) return;

    QCanBusFrame frame;
    frame.setFrameId(0x1CFF0100); // ID de ejemplo J1939 prioritario para Paro de Emergencia
    frame.setExtendedFrameFormat(true);

    // Payload con comando de texto o flags específicos según protocolo de la ECU
    QByteArray payload = "STOP_ALL";
    frame.setPayload(payload);

    m_canDevice->writeFrame(frame);
    qInfo() << "🛰️ CAN Bus: Trama de Paro Total enviada al bus.";
}

void CanBusDataSource::sendStopShovelCommand()
{
    if (!m_canDevice || m_canDevice->state() != QCanBusDevice::ConnectedState) return;

    QCanBusFrame frame;
    frame.setFrameId(0x1CFF0200); // ID de ejemplo J1939 prioritario para Bloqueo de Válvulas
    frame.setExtendedFrameFormat(true);

    QByteArray payload = "STP_SHVL";
    frame.setPayload(payload);

    m_canDevice->writeFrame(frame);
    qInfo() << "🛰️ CAN Bus: Trama de Bloqueo Hidráulico enviada al bus.";
}

void CanBusDataSource::handleDeviceError(QCanBusDevice::CanBusError error)
{
    if (m_canDevice) {
        emit connectionError(QString("Fallo crítico en hardware CAN (Código %1): %2")
                                 .arg(error)
                                 .arg(m_canDevice->errorString()));
    }
}
