#include "excavatorcontroller.h"
#include "configmanager.h"
#include <QDebug>
#include <QDateTime>

// SOLUCIÓN AL ERROR CRÍTICO: Añadido m_sqlService(sqlService) a la lista de inicialización
ExcavatorController::ExcavatorController(ExcavatorDataSource *source, SQLService *sqlService, QObject *parent)
    : QObject(parent), m_source(source), m_sqlService(sqlService)
{
    // Configuremos la base de datos para guardar los eventos de forma segura
    if (m_sqlService) {
        m_alarmHistory = m_sqlService->loadAllAlarms();
        emit alarmHistoryChanged();
        qInfo() << "✅ ExcavatorController: Historial recuperado mediante SQLService. Registros:" << m_alarmHistory.size();
    }

    // Configuremos el watchdog timer
    m_watchdogTimer = new QTimer(this);
    m_watchdogTimer->setInterval(1000);
    m_watchdogTimer->setSingleShot(true);
    connect(m_watchdogTimer, &QTimer::timeout, this, [this]() {
        if (!m_commLoss) {
            m_commLoss = true;
            emit commLossChanged();
            logEvent("Sistema", "FALLA CRÍTICA", "Pérdida total de comunicación con la máquina");
        }
    });
    m_watchdogTimer->start();

    // LECTURA DINÁMICA DE SENSORES
    connect(source, &ExcavatorDataSource::sensorDataReceived, this, [this](const QString &sensorId, double value) {
        m_watchdogTimer->start();

        if (m_commLoss) {
            m_commLoss = false;
            emit commLossChanged();
            logEvent("Sistema", "CONEXIÓN", "Comunicación reestablecida con éxito");
        }

        // Verificar si el valor cambió para evitar sobrecarga
        if (m_sensors.contains(sensorId) && qFuzzyCompare(m_sensors[sensorId].toDouble(), value)) {
            return;
        }

        // Guardar en el mapa dinámico y notificar a QML (Para las tarjetas autogeneradas)
        m_sensors[sensorId] = value;
        emit sensorsChanged();

        // -------------------------------------------------------------
        // SOLUCIÓN DE LÓGICA: Sincronización con las alertas de la HMI
        // -------------------------------------------------------------
        double maxLimit = ConfigManager::getDynamicLimit(sensorId + "_max");
        double minLimit = ConfigManager::getDynamicLimit(sensorId + "_min");

        // Evaluar si la condición actual de este sensor es crítica
        bool isCritical = false;
        QString alertDetail;

        if (maxLimit != -1.0 && value >= maxLimit) {
            isCritical = true;
            alertDetail = QString("Límite Máximo superado: %1 (Máx: %2)").arg(value, 0, 'f', 1);
        } else if (minLimit != -1.0 && value <= minLimit) {
            isCritical = true;
            alertDetail = QString("Límite Mínimo superado: %1 (Mín: %2)").arg(value, 0, 'f', 1);
        }

        // MAREAR SENSORES ESPECÍFICOS A LAS PROPIEDADES FIJAS DE ALERTAS DE LA PANTALLA
        if (sensorId == "inclinacion") {
            // Actualizar el ángulo para el visualizador 2D que rota
            m_inclination = value;
            emit inclinationChanged();

            if (m_alertTilt != isCritical) {
                m_alertTilt = isCritical;
                emit alertTiltChanged();

                if (m_alertTilt) {
                    logEvent("Inclinación", "ACTIVADA", alertDetail);
                } else {
                    m_tiltAck = false; // Reset del botón ACK
                    logEvent("Inclinación", "NORMALIZADA", QString("Ángulo seguro: %1°").arg(value, 0, 'f', 1));
                }
            }
        }
        else if (sensorId == "distancia_brazo") {
            // Actualizar la distancia para el cálculo cinemático del brazo en QML
            m_armDistance = value;
            emit armDistanceChanged();

            if (m_alertCrash != isCritical) {
                m_alertCrash = isCritical;
                emit alertCrashChanged();

                if (m_alertCrash) {
                    logEvent("Proximidad Brazo", "ACTIVADA", alertDetail);
                } else {
                    m_crashAck = false; // Reset del botón ACK
                    logEvent("Proximidad Brazo", "NORMALIZADA", QString("Distancia segura: %1m").arg(value, 0, 'f', 2));
                }
            }
        }
        else {
            // Como no tienen una propiedad fija en QML, solo guardamos su activación/normalización en el log de SQLite
            // Usamos una bandera temporal guardada en memoria dinámica para rastrear si ya se reportó
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

    // Control de errores físico/red
    connect(source, &ExcavatorDataSource::connectionError, this, [](const QString &err) {
        qWarning() << "❌ Error Crítico de Hardware HMI:" << err;
    });
}

double ExcavatorController::getSensorValue(const QString &sensorId) const {
    return m_sensors.value(sensorId, 0.0).toDouble();
}

void ExcavatorController::logEvent(const QString &type, const QString &state, const QString &details) {
    QString timeStr = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");

    if (m_sqlService) {
        m_sqlService->saveAlarm(timeStr, type, state, details);
    }

    QVariantMap event;
    event["timestamp"] = timeStr;
    event["type"] = type;
    event["state"] = state;
    event["details"] = details;

    m_alarmHistory.prepend(event);
    emit alarmHistoryChanged();
}

void ExcavatorController::requestEngineShutdown() {
    m_source->sendStopAllCommand();
    logEvent("Sistema", "COMANDO ENVIADO", "Apagar motor por inclinación crítica");
}

void ExcavatorController::requestShovelLock() {
    m_source->sendStopShovelCommand();
    logEvent("Sistema", "COMANDO ENVIADO", "Bloqueo de pala por proximidad");
}

double ExcavatorController::getDynamicLimit(const QString &limitKey) const {
    // Redirige la consulta directamente hacia el ConfigManager de C++
    return ConfigManager::getDynamicLimit(limitKey);
}