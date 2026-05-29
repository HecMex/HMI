#include "canbusdatasource.h"
#include "configmanager.h"
#include <QCanBus>
#include <QDataStream>
#include <QDebug>

/**
 * @brief Constructor de la clase CanBusDataSource.
 * @param parent Puntero al objeto QObject padre para la gestión automática de memoria.
 */
CanBusDataSource::CanBusDataSource(QObject *parent)
    : ExcavatorDataSource(parent), m_canDevice(nullptr) {}

/**
 * @brief Destructor de la clase CanBusDataSource.
 * Garantiza la liberación y desconexión segura del hardware antes de destruir el objeto.
 */
CanBusDataSource::~CanBusDataSource() {
    CanBusDataSource::disconnectSource();
}

/**
 * @brief Inicializa el dispositivo de hardware y se conecta al Bus CAN real o virtual.
 *
 * Este método valida el controlador (plugin) configurado en el JSON, instancia el
 * dispositivo de red correspondiente y enlaza las señales internas de recepción de datos.
 *
 * @return true si la HMI se conectó exitosamente al bus físico; false en caso de error.
 */
bool CanBusDataSource::initialize() {
    // Verificar si el plugin CAN especificado en config.json (ej. socketcan, slcan) existe en el sistema
    if (!QCanBus::instance()->plugins().contains(ConfigManager::canPlugin)) {
        emit connectionError(QString("Plugin CAN no disponible: %1").arg(ConfigManager::canPlugin));
        return false;
    }

    QString errorStr;
    // Intentar instanciar el canal físico (ej. interfaz "can0" o puerto "COM4")
    m_canDevice = QCanBus::instance()->createDevice(ConfigManager::canPlugin, ConfigManager::canInterface, &errorStr);

    if (!m_canDevice) {
        emit connectionError("Error al crear dispositivo CAN: " + errorStr);
        return false;
    }

    // Enlazar los slots para capturar eventos de recepción de tramas y errores de hardware
    connect(m_canDevice, &QCanBusDevice::framesReceived, this, &CanBusDataSource::readCanFrames);
    connect(m_canDevice, &QCanBusDevice::errorOccurred, this, &CanBusDataSource::handleDeviceError);

    // Intentar realizar el enganche eléctrico/lógico al bus físico
    bool success = m_canDevice->connectDevice();
    if (success) {
        qInfo() << "✅ Conectado con éxito al Bus CAN Industrial:" << ConfigManager::canInterface;
    } else {
        emit connectionError("Fallo al conectar al bus CAN físico: " + m_canDevice->errorString());
    }
    return success;
}

/**
 * @brief Desconecta el dispositivo de hardware y libera los recursos del puerto.
 *
 * Cierra la sesión activa con el bus del sistema y destruye de forma segura el puntero
 * para evitar fugas de memoria (Memory Leaks) u ocupación del puerto serial.
 */
void CanBusDataSource::disconnectSource() {
    if (m_canDevice) {
        // Apagar la comunicación lógica si se encuentra enlazado
        if (m_canDevice->state() == QCanBusDevice::ConnectedState) {
            m_canDevice->disconnectDevice();
        }
        delete m_canDevice;
        m_canDevice = nullptr;
        qInfo() << "🔌 Bus CAN desconectado de forma segura.";
    }
}

/**
 * @brief Slot encargado de leer, decodificar e interpretar las tramas binarias del Bus CAN.
 *
 * Se ejecuta de forma asíncrona cada vez que el hardware notifica la llegada de bytes.
 * Realiza una traducción asociativa (Data-Driven) convirtiendo IDs numéricos en cadenas de
 * texto (IDs de sensor) y desempaqueta payloads binarios Little-Endian de 8 bytes de precisión.
 */
void CanBusDataSource::readCanFrames() {
    if (!m_canDevice) return;

    // Procesar cíclicamente todas las tramas acumuladas en el búfer de recepción de hardware
    while (m_canDevice->framesAvailable()) {
        const QCanBusFrame frame = m_canDevice->readFrame();

        // 1. Convertir el ID numérico binario de la trama a formato de texto Hexadecimal estándar de 8 dígitos ("0x1CFDD600")
        QString frameIdStr = QString("0x%1").arg(frame.frameId(), 8, 16, QChar('0')).toUpper();

        // 2. Verificar si el ID de la trama CAN que viaja por el cable está registrado en el mapa de traducción del config.json
        if (ConfigManager::canMapping.contains(frameIdStr)) {
            // Recuperar el ID alfanumérico del sensor (ej. "inclinacion", "temp_motor")
            QString sensorId = ConfigManager::canMapping.value(frameIdStr);
            QByteArray payload = frame.payload();

            // 3. Desempaquetar el flujo binario crudo (El estándar de datos requiere mínimo 8 bytes para un double)
            if (payload.size() >= sizeof(double)) {
                QDataStream stream(payload);
                stream.setByteOrder(QDataStream::LittleEndian); // Forzar Little-Endian para coincidir con el script de Python
                stream.setFloatingPointPrecision(QDataStream::DoublePrecision); // Forzar precisión de punto flotante de 64 bits (double)

                double value = 0.0;
                stream >> value; // Extraer los bytes decodificados hacia la variable analítica

                // 4. Emitir la señal polimórfica genérica que procesa y mapea la lógica en ExcavatorController
                emit sensorDataReceived(sensorId, value);
            }
        }
    }
}

/**
 * @brief Inyecta una trama de control de alta prioridad al bus para apagar el motor principal.
 *
 * Envía un comando inmediato utilizando el estándar de formato extendido de 29 bits (J1939)
 * direccionado a las ECUs del chasis de la excavadora real.
 */
void CanBusDataSource::sendStopAllCommand()
{
    // Validar que el hardware de red esté disponible y encendido antes de transmitir
    if (!m_canDevice || m_canDevice->state() != QCanBusDevice::ConnectedState) return;

    QCanBusFrame frame;
    frame.setFrameId(0x1CFF0100); // ID extendido prioritario de J1939 para Paro Crítico de Emergencia
    frame.setExtendedFrameFormat(true);

    // Definir la carga útil formateada según especificaciones de la ECU de destino
    QByteArray payload = "STOP_ALL";
    frame.setPayload(payload);

    // Escribir la trama directamente en las líneas del bus físico
    m_canDevice->writeFrame(frame);
    qInfo() << "🛰️ CAN Bus: Trama de Paro Total enviada al bus.";
}

/**
 * @brief Inyecta una trama al bus para interrumpir el flujo y bloquear las válvulas hidráulicas de la pala.
 *
 * Diseñado como mecanismo preventivo independiente para congelar el movimiento del brazo articulado
 * en caso de riesgo inminente de impacto contra el parabrisas de la cabina.
 */
void CanBusDataSource::sendStopShovelCommand()
{
    if (!m_canDevice || m_canDevice->state() != QCanBusDevice::ConnectedState) return;

    QCanBusFrame frame;
    frame.setFrameId(0x1CFF0200); // ID extendido prioritario de J1939 para Bloqueo de Actuadores Hidráulicos
    frame.setExtendedFrameFormat(true);

    QByteArray payload = "STP_SHVL";
    frame.setPayload(payload);

    m_canDevice->writeFrame(frame);
    qInfo() << "🛰️ CAN Bus: Trama de Bloqueo Hidráulico enviada al bus.";
}

/**
 * @brief Slot encargado de interceptar y reportar anomalías físicas o eléctricas del bus de hardware.
 * @param error Código numérico de enumeración que identifica la falla (ej. sobrecarga, desconexión de cable).
 */
void CanBusDataSource::handleDeviceError(QCanBusDevice::CanBusError error)
{
    if (m_canDevice) {
        // Emitir el error hacia el controlador para suspender temporalmente la HMI y registrar la falla en SQLite
        emit connectionError(QString("Fallo crítico en hardware CAN (Código %1): %2")
                                 .arg(error)
                                 .arg(m_canDevice->errorString()));
    }
}
