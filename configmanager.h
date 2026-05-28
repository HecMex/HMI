#ifndef CONFIGMANAGER_H
#define CONFIGMANAGER_H

#include <QString>
#include <QMap>

class ConfigManager {
public:
    static bool loadConfig(const QString &filePath);

    static double getDynamicLimit(const QString& limitKey);

    static QString mode; // "simulation" o "production"

    // Parámetros UDP
    static int udpPort;
    static QString udpAddress;

    // Parámetros CAN
    static QString canInterface;
    static QString canPlugin;
    static unsigned int canTelemetryId;

    // Parámetros de Alarma
    static double maxTilt;
    static double minDistance;

    // Contenedor global para asociar IDs de CAN (Hex strings) con IDs de Sensores (QString)
    static QMap<QString, QString> canMapping;

private:
    static QMap<QString, double> m_dynamicAlarms;
};

QDebug operator<<(QDebug debug, const ConfigManager &);

#endif // CONFIGMANAGER_H
