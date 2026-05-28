#include "configmanager.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QDebug>

// Inicialización de variables estáticas por defecto
QString ConfigManager::mode = "simulation";
int ConfigManager::udpPort = 5005;
QString ConfigManager::udpAddress = "127.0.0.1";
QString ConfigManager::canInterface = "can0";
QString ConfigManager::canPlugin = "socketcan";
unsigned int ConfigManager::canTelemetryId = 0x1CFDD600;
double ConfigManager::maxTilt = 25.0;
double ConfigManager::minDistance = 0.8;
QMap<QString, double> ConfigManager::m_dynamicAlarms;
QMap<QString, QString> ConfigManager::canMapping;

bool ConfigManager::loadConfig(const QString &filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "⚠️ No se pudo abrir config.json. Usando valores por defecto.";
        return false;
    }

    QByteArray rawData = file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(rawData, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        qWarning() << "⚠️ Error de parseo en JSON:" << parseError.errorString();
        return false;
    }

    QJsonObject rootObj = doc.object();

    if (rootObj.contains("mode")) ConfigManager::mode = rootObj["mode"].toString();

    if (rootObj.contains("udp")) {
        QJsonObject udpObj = rootObj["udp"].toObject();
        if (udpObj.contains("port")) ConfigManager::udpPort = udpObj["port"].toInt();
        if (udpObj.contains("address")) ConfigManager::udpAddress = udpObj["address"].toString();
    }

    if (rootObj.contains("can")) {
        QJsonObject canObj = rootObj["can"].toObject();
        if (canObj.contains("interface")) ConfigManager::canInterface = canObj["interface"].toString();
        if (canObj.contains("plugin")) ConfigManager::canPlugin = canObj["plugin"].toString();
        if (canObj.contains("telemetry_id")) {
            ConfigManager::canTelemetryId = canObj["telemetry_id"].toString().toUInt(nullptr, 16);
        }
        if (canObj.contains("mapping")) {
            QJsonObject mapObj = canObj["mapping"].toObject();
            ConfigManager::canMapping.clear();
            for (auto it = mapObj.constBegin(); it != mapObj.constEnd(); ++it) {
                ConfigManager::canMapping.insert(it.key().toUpper(), it.value().toString());
                qInfo() << "   🛰️ Mapeo CAN registrado ->" << it.key() << ":" << it.value().toString();
            }
        }
    }

    // LECTURA DINÁMICA DE ALARMAS
    if (rootObj.contains("alarms")) {
        QJsonObject alarmsObj = rootObj["alarms"].toObject();

        // Limpiamos registros previos si existieran antes de recargar
        ConfigManager::m_dynamicAlarms.clear();

        // Iteramos sobre cada una de las llaves declaradas en el bloque alarms del JSON
        for (auto it = alarmsObj.constBegin(); it != alarmsObj.constEnd(); ++it) {
            QString key = it.key();
            double value = it.value().toDouble();

            ConfigManager::m_dynamicAlarms.insert(key, value);
            qInfo() << "   🔍 Límite registrado ->" << key << ":" << value;
        }
    }

    qInfo() << "✅ Configuración JSON mapeada con éxito en modo:" << ConfigManager::mode;
    return true;
}

QDebug operator<<(QDebug debug, const ConfigManager &) {
    QDebugStateSaver saver(debug); // Protege el estado previo del formato de QDebug

    debug.nospace() << "ConfigManager(\n"
                    << "  Mode: " << ConfigManager::mode << "\n";

    if (ConfigManager::mode == "simulation") {
        debug << "  UDP Port: " << ConfigManager::udpPort << "\n"
              << "  UDP Address: " << ConfigManager::udpAddress << "\n";
    } else {
        // CORRECCIÓN: Usamos Qt::hex para activar hexadecimal y agregamos formato manual legible
        QString hexId = QString("0x%1").arg(ConfigManager::canTelemetryId, 8, 16, QChar('0')).toUpper();

        debug << "  CAN Interface: " << ConfigManager::canInterface << "\n"
              << "  CAN Plugin: " << ConfigManager::canPlugin << "\n"
              << "  CAN Telemetry ID: " << hexId << "\n";
    }

    debug << "  Max Tilt: " << ConfigManager::maxTilt << "°\n"
          << "  Min Distance: " << ConfigManager::minDistance << "m\n"
          << ")";

    return debug;
}

double ConfigManager::getDynamicLimit(const QString &limitKey) {
    // Si la llave solicitada existe en el mapa, regresamos su valor.
    // Si no tiene regla configurada, regresa -1.0 como bandera de omision.
    return ConfigManager::m_dynamicAlarms.value(limitKey, -1.0);
}