#include "excavatorcontroller.h"
#include "configmanager.h"
#include <QDebug>
#include <QDateTime>

/**
 * @brief Constructor de la clase ExcavatorController.
 *
 * Configura el estado inicial de la HMI mediante inyección de dependencias (origen de datos
 * y motor SQL), inicializa el Watchdog de seguridad de la red y establece el lazo reactivo
 * principal para el procesamiento de telemetría dinámica mediante lambdas de C++.
 *
 * @param source Puntero polimórfico a la interfaz abstracta del origen de datos (UDP o CAN).
 * @param sqlService Puntero al servicio de persistencia local en base de datos SQLite.
 * @param parent Puntero al objeto QObject padre para la gestión de memoria interna de Qt.
 */
ExcavatorController::ExcavatorController(ExcavatorDataSource *source, SQLService *sqlService, QObject *parent)
    : QObject(parent), m_source(source), m_sqlService(sqlService)
{
    // Inicializar y cargar de forma síncrona el histórico permanente de alarmas desde SQLite
    if (m_sqlService) {
        m_alarmHistory = m_sqlService->loadAllAlarms();
        emit alarmHistoryChanged(); // Notificar al ListView de QML para renderizar las filas iniciales
        qInfo() << "✅ ExcavatorController: Historial recuperado mediante SQLService. Registros:" << m_alarmHistory.size();
    }

    // Configurar e iniciar el Watchdog Timer de la red de telemetría
    m_watchdogTimer = new QTimer(this);
    m_watchdogTimer->setInterval(1000); // Ventana de tolerancia máxima: 1 segundo (1000 ms)
    m_watchdogTimer->setSingleShot(true); // Evitar disparos cíclicos repetitivos

    connect(m_watchdogTimer, &QTimer::timeout, this, [this]() {
        // Se ejecuta si transcurre 1 segundo completo sin recibir tramas del bus/red
        if (!m_commLoss) {
            m_commLoss = true;
            emit commLossChanged(); // Desplegar la capa de bloqueo global en QML (commLossOverlay)
            logEvent("Sistema", "FALLA CRÍTICA", "Pérdida total de comunicación con la máquina");
        }
    });
    m_watchdogTimer->start();

    // LAZO REACTIVO CENTRAL: Procesamiento de tramas e inyección de datos
    connect(source, &ExcavatorDataSource::sensorDataReceived, this, [this](const QString &sensorId, double value) {
        // Alimentar al watch dog en cada ráfaga recibida: la máquina sigue viva
        m_watchdogTimer->start();

        // Si el sistema venía de una condición de desconexión, restablecer el acceso a la HMI
        if (m_commLoss) {
            m_commLoss = false;
            emit commLossChanged(); // Ocultar la capa de bloqueo en QML
            logEvent("Sistema", "CONEXIÓN", "Comunicación reestablecida con éxito");
        }

        // Optimización de rendimiento: Filtrar valores repetidos mediante comparación de punto flotante tolerante
        if (m_sensors.contains(sensorId) && qFuzzyCompare(m_sensors[sensorId].toDouble(), value)) {
            return;
        }

        // Almacenar el valor en la estructura asociativa en RAM y notificar al Repeater de QML
        m_sensors[sensorId] = value;
        emit sensorsChanged(); // Forzar la actualización visual del TelemetryPanel.qml dinámico

        // EVALUACIÓN DINÁMICA DE REGLAS DE SEGURIDAD (LÍMITES)
        double maxLimit = ConfigManager::getDynamicLimit(sensorId + "_max");
        double minLimit = ConfigManager::getDynamicLimit(sensorId + "_min");

        bool isCritical = false;
        QString alertDetail;

        // Comprobación de límites superiores configurados en el JSON
        if (maxLimit != -1.0 && value >= maxLimit) {
            isCritical = true;
            alertDetail = QString("Límite Máximo superado: %1 (Máx: %2)").arg(value, 0, 'f', 1);
        }
        // Comprobación de límites inferiores configurados en el JSON
        else if (minLimit != -1.0 && value <= minLimit) {
            isCritical = true;
            alertDetail = QString("Límite Mínimo superado: %1 (Mín: %2)").arg(value, 0, 'f', 1);
        }

        // SINCRONIZACIÓN INDUSTRIAL: Vincular datos dinámicos a los widgets estáticos/animaciones de QML
        if (sensorId == "inclinacion") {
            m_inclination = value;
            emit inclinationChanged(); // Actualizar la rotación del ExcavatorVisualizer.qml

            // Control de transiciones de estado de la alarma
            if (m_alertTilt != isCritical) {
                m_alertTilt = isCritical;
                emit alertTiltChanged(); // Desplegar/Ocultar botón de paro rápido en AlertPanel.qml

                if (m_alertTilt) {
                    logEvent("Inclinación", "ACTIVADA", alertDetail);
                } else {
                    m_tiltAck = false; // Resetear la confirmación del operador al normalizarse el ángulo
                    logEvent("Inclinación", "NORMALIZADA", QString("Ángulo seguro: %1°").arg(value, 0, 'f', 1));
                }
            }
        }
        else if (sensorId == "distancia_brazo") {
            m_armDistance = value;
            emit armDistanceChanged(); // Actualizar la extensión cinemática del brazo 2D en pantalla

            if (m_alertCrash != isCritical) {
                m_alertCrash = isCritical;
                emit alertCrashChanged();

                if (m_alertCrash) {
                    logEvent("Proximidad Brazo", "ACTIVADA", alertDetail);
                } else {
                    m_crashAck = false; // Resetear la confirmación del operador al alejarse del vidrio
                    logEvent("Proximidad Brazo", "NORMALIZADA", QString("Distancia segura: %1m").arg(value, 0, 'f', 2));
                }
            }
        }
        else {
            // PROCESAMIENTO COMPORTAMIENTO SENSORES GENERALES
            // Se gestionan mediante un mapa estático interno para registrar la transición de eventos
            static QMap<QString, bool> genericAlertStates;
            if (genericAlertStates.value(sensorId, false) != isCritical) {
                genericAlertStates[sensorId] = isCritical;
                if (isCritical) {
                    logEvent(sensorId, "ACTIVADA", alertDetail);
                } else {
                    logEvent(sensorId, "NORMALIZADA", QString("Valor seguro: %1").arg(value));
                }
            }
        }
    });

    // Interceptación y reporte de fallas físicas/eléctricas de los puertos del sistema
    connect(source, &ExcavatorDataSource::connectionError, this, [](const QString &err) {
        qWarning() << "❌ Error Crítico de Hardware HMI:" << err;
    });
}

/**
 * @brief Recupera de forma síncrona el valor actual de un sensor guardado en la memoria RAM.
 * @param sensorId Identificador único del sensor (ej. "temp_motor").
 * @return double El valor almacenado; retorna 0.0 si el sensor aún no ha transmitido datos.
 */
double ExcavatorController::getSensorValue(const QString &sensorId) const {
    return m_sensors.value(sensorId, 0.0).toDouble();
}

/**
 * @brief Registra y estampa cronológicamente una alarma tanto en la RAM (para QML) como en el disco duro (SQLite).
 *
 * @param type Clasificación del evento o ID del componente (ej. "Sistema", "Inclinación").
 * @param state Estado operativo de la alarma ("ACTIVADA", "RECONOCIDA", "NORMALIZADA").
 * @param details Información técnica legible sobre las lecturas numéricas instantáneas.
 */
void ExcavatorController::logEvent(const QString &type, const QString &state, const QString &details) {
    // Generar estampa de tiempo con precisión de segundos según estándar de auditorías industriales
    QString timeStr = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");

    // Delegar la inserción asíncrona permanente en disco duro al SQLService
    if (m_sqlService) {
        m_sqlService->saveAlarm(timeStr, type, state, details);
    }

    // Encapsular los datos en un mapa compatible con el formato de objetos de Javascript en QML
    QVariantMap event;
    event["timestamp"] = timeStr;
    event["type"] = type;
    event["state"] = state;
    event["details"] = details;

    // Insertar al principio (índice 0) para que las alarmas más recientes fluyan a la parte superior de la tabla
    m_alarmHistory.prepend(event);
    emit alarmHistoryChanged(); // Notificar al ListView de QML para redibujar de forma instantánea
}

/**
 * @brief Método disparado por el operador desde QML para ordenar el apagado total de la excavadora.
 * Invoca el canal de salida de hardware e inserta la acción de mando en la auditoría persistente.
 */
void ExcavatorController::requestEngineShutdown() {
    m_source->sendStopAllCommand(); // Despachar comando por la red UDP o bus CAN
    logEvent("Sistema", "COMANDO ENVIADO", "Apagar motor por inclinación crítica");
}

/**
 * @brief Método disparado por el operador desde QML para bloquear el flujo hidráulico de la pala.
 */
void ExcavatorController::requestShovelLock() {
    m_source->sendStopShovelCommand();
    logEvent("Sistema", "COMANDO ENVIADO", "Bloqueo de pala por proximidad");
}

/**
 * @brief Función puente para exponer las reglas límite del JSON hacia el front-end desacoplado.
 *
 * Permite que los archivos QML realicen consultas reactivas sobre umbrales sin importar
 * la sintaxis o procedencia de la clase ConfigManager de C++.
 *
 * @param limitKey Sufijo de la alarma a consultar (ej: "temp_motor_max").
 * @return double El umbral configurado en disco duro; -1.0 si no existe regla aplicada.
 */
double ExcavatorController::getDynamicLimit(const QString &limitKey) const {
    return ConfigManager::getDynamicLimit(limitKey);
}
