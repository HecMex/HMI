#ifndef EXCAVATORCONTROLLER_H
#define EXCAVATORCONTROLLER_H

#include <QObject>
#include <QUdpSocket>
#include <QVariantList>
#include <QTimer>

#include "sqlservice.h"
#include "excavatordatasource.h"

class ExcavatorController : public QObject {
    Q_OBJECT

    Q_PROPERTY(double inclination READ inclination NOTIFY inclinationChanged)
    Q_PROPERTY(double armDistance READ armDistance NOTIFY armDistanceChanged)
    Q_PROPERTY(bool alertTilt READ alertTilt NOTIFY alertTiltChanged)
    Q_PROPERTY(bool alertCrash READ alertCrash NOTIFY alertCrashChanged)
    Q_PROPERTY(QVariantList alarmHistory READ alarmHistory NOTIFY alarmHistoryChanged)
    Q_PROPERTY(bool commLoss READ commLoss NOTIFY commLossChanged)
    Q_PROPERTY(QVariantMap sensors READ sensors NOTIFY sensorsChanged)

public:
    Q_INVOKABLE void requestEngineShutdown();
    Q_INVOKABLE void requestShovelLock();
    Q_INVOKABLE double getSensorValue(const QString& sensorId) const;
    Q_INVOKABLE double getDynamicLimit(const QString& limitKey) const;

    explicit ExcavatorController(ExcavatorDataSource *source, SQLService* sqlService, QObject* parent = nullptr);

    double inclination() const { return m_inclination; }
    double armDistance() const { return m_armDistance; }
    bool alertTilt() const { return m_alertTilt; }
    bool alertCrash() const { return m_alertCrash; }
    QVariantList alarmHistory() const { return m_alarmHistory; }
    bool commLoss() const { return m_commLoss; }
    QVariantMap sensors() const { return m_sensors; }

signals:
    void inclinationChanged();
    void armDistanceChanged();
    void alertTiltChanged();
    void alertCrashChanged();
    void alarmHistoryChanged();
    void commLossChanged();
    void sensorsChanged();

private:
    QUdpSocket *udpSocket;
    double m_inclination = 0.0;
    double m_armDistance = 5.0;
    bool m_alertTilt = false;
    bool m_alertCrash = false;
    bool m_tiltAck = false;
    bool m_crashAck = false;
    QVariantList m_alarmHistory;
    ExcavatorDataSource *m_source;
    QTimer* m_watchdogTimer;
    bool m_commLoss = false;
    SQLService* m_sqlService;
    QVariantMap m_sensors;

    void logEvent(const QString &type, const QString &state, const QString &details);
};

#endif // EXCAVATORCONTROLLER_H
