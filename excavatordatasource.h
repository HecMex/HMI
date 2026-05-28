#ifndef EXCAVATORDATASOURCE_H
#define EXCAVATORDATASOURCE_H

#include <QObject>

class ExcavatorDataSource : public QObject {
    Q_OBJECT
public:
    explicit ExcavatorDataSource(QObject *parent = nullptr) : QObject(parent) {}
    virtual ~ExcavatorDataSource() {}

    virtual bool initialize() = 0;
    virtual void disconnectSource() = 0;
    virtual void sendStopAllCommand() = 0;
    virtual void sendStopShovelCommand() = 0;

signals:
    // NUEVA SEÑAL GENÉRICA: Soporta CUALQUIER sensor presente o futuro
    void sensorDataReceived(const QString &sensorId, double value);
    void connectionError(const QString &errorMsg);
};

#endif // EXCAVATORDATASOURCE_H
