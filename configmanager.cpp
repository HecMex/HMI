#include "configmanager.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QDebug>

// INICIALIZACIÓN DE VARIABLES ESTÁTICAS (Valores por default)
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

/**
 * @brief Carga, parsea e inyecta en memoria el archivo de configuración JSON externo.
 *
 * Este método abre el archivo en modo lectura de texto, valida que el formato
 * estructural sea un JSON válido y extrae de forma segura las configuraciones para
 * los subsistemas UDP, CAN Bus y el mapa asociativo de alarmas dinámicas.
 *
 * @param filePath Ruta física absoluta o relativa del archivo config.json en el disco.
 * @return true si la configuración se leyó e inyectó con éxito; false si el archivo
 *         no existe o contiene errores de sintaxis JSON.
 */
bool ConfigManager::loadConfig(const QString &filePath) {
    QFile file(filePath);
    // Intentar abrir el archivo de texto en modo de solo lectura
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "⚠️ No se pudo abrir config.json. Usando valores por defecto.";
        return false;
    }

    // Leer todo el contenido crudo del archivo y cerrar el descriptor del sistema
    QByteArray rawData = file.readAll();
    file.close();

    // Validar estructuralmente la sintaxis del JSON
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(rawData, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        qWarning() << "⚠️ Error de parseo en JSON:" << parseError.errorString();
        return false;
    }

    // Convertir el documento a un objeto JSON raíz manipulable
    QJsonObject rootObj = doc.object();

    // Extraer el modo operativo principal de la HMI ("simulation" o "production")
    if (rootObj.contains("mode")) ConfigManager::mode = rootObj["mode"].toString();

    // Configuración de red local UDP, si se encuentra
    if (rootObj.contains("udp")) {
        QJsonObject udpObj = rootObj["udp"].toObject();
        if (udpObj.contains("port")) ConfigManager::udpPort = udpObj["port"].toInt();
        if (udpObj.contains("address")) ConfigManager::udpAddress = udpObj["address"].toString();
    }

    // Configuración de hardware e IDs de traducción CAN Bus (J1939), si se encuentra
    if (rootObj.contains("can")) {
        QJsonObject canObj = rootObj["can"].toObject();
        if (canObj.contains("interface")) ConfigManager::canInterface = canObj["interface"].toString();
        if (canObj.contains("plugin")) ConfigManager::canPlugin = canObj["plugin"].toString();

        // Conversión segura de ID string hexadecimal ("0x1CFDD600") a un entero sin signo de 32 bits
        if (canObj.contains("telemetry_id")) {
            ConfigManager::canTelemetryId = canObj["telemetry_id"].toString().toUInt(nullptr, 16);
        }

        // Extracción asociativa del diccionario de mapeo ID CAN <-> ID Sensor
        if (canObj.contains("mapping")) {
            QJsonObject mapObj = canObj["mapping"].toObject();
            ConfigManager::canMapping.clear(); // Limpiar registros de la sesión previa

            for (auto it = mapObj.constBegin(); it != mapObj.constEnd(); ++it) {
                // Almacenar las llaves en mayúsculas para evitar colisiones de formato hex en el bus
                ConfigManager::canMapping.insert(it.key().toUpper(), it.value().toString());
                qInfo() << "   🛰️ Mapeo CAN registrado ->" << it.key() << ":" << it.value().toString();
            }
        }
    }

    // Límites de seguridad y alarmas dinámicas
    if (rootObj.contains("alarms")) {
        QJsonObject alarmsObj = rootObj["alarms"].toObject();

        // Limpiar el mapa estático antes de inyectar las nuevas reglas del disco
        ConfigManager::m_dynamicAlarms.clear();

        // Iterar de forma automatizada sobre cada regla declarada en el JSON sin usar variables fijas
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

/**
 * @brief Sobrecarga global del operador de flujo para la consola de depuración de Qt (QDebug).
 *
 * Permite imprimir el estado actual completo de la configuración llamando simplemente a
 * `qDebug() << ConfigManager();` en cualquier parte del código fuente. Formatea de
 * forma inteligente los bloques impresos discriminando si está en modo UDP o CAN.
 *
 * @param debug Flujo de depuración nativo de Qt.
 * @param Object Referencia constante al objeto gestor de configuración.
 * @return QDebug Retorna el flujo modificado con los textos formateados en cascada.
 */
QDebug operator<<(QDebug debug, const ConfigManager &) {
    // QDebugStateSaver encapsula y restaura el formato previo de consola para no alterar otros logs
    QDebugStateSaver saver(debug);

    debug.nospace() << "ConfigManager(\n"
                    << "  Mode: " << ConfigManager::mode << "\n";

    // Formatear la impresión basándose exclusivamente en el canal activo de datos
    if (ConfigManager::mode == "simulation") {
        debug << "  UDP Port: " << ConfigManager::udpPort << "\n"
              << "  UDP Address: " << ConfigManager::udpAddress << "\n";
    } else {
        // Formatear el ID binario de CAN para representarlo visualmente como un hexadecimal de 8 dígitos con prefijo 0x
        QString hexId = QString("0x%1").arg(ConfigManager::canTelemetryId, 8, 16, QChar('0')).toUpper();

        debug << "  CAN Interface: " << ConfigManager::canInterface << "\n"
              << "  CAN Plugin: " << ConfigManager::canPlugin << "\n"
              << "  CAN Telemetry ID: " << hexId << "\n";
    }

    // Imprimir los umbrales de las variables fijas base
    debug << "  Max Tilt: " << ConfigManager::maxTilt << "°\n"
          << "  Min Distance: " << ConfigManager::minDistance << "m\n"
          << ")";

    return debug;
}

/**
 * @brief Recupera de forma segura el límite analítico asignado a un sensor desde el mapa dinámico.
 *
 * Es consumido de manera polimórfica por el ExcavatorController para evaluar condiciones de peligro
 * adjuntando sufijos de tipo de umbral (ej: "sensorId + _max" o "sensorId + _min").
 *
 * @param limitKey Clave alfanumérica única de la regla de alarma (ej. "temp_motor_max").
 * @return double El umbral numérico si la clave existe en el JSON; -1.0 actúa como bandera
 *                de omisión si el sensor no requiere alertas visuales.
 */
double ConfigManager::getDynamicLimit(const QString &limitKey) {
    // Retorna el valor si existe; si no, retorna el valor por defecto seguro (-1.0)
    return ConfigManager::m_dynamicAlarms.value(limitKey, -1.0);
}
