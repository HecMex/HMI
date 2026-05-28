#ifndef UDPDATASOURCE_H
#define UDPDATASOURCE_H

#include "excavatordatasource.h"
#include <QUdpSocket>

class UdpDataSource : public ExcavatorDataSource {
    Q_OBJECT
public:
    static const QHostAddress pythonAddress;
    static const int pythonPort;

    UdpDataSource(QObject *parent = nullptr) : ExcavatorDataSource(parent), udpSocket(nullptr) {}

    bool initialize() override;

    inline void disconnectSource() override { if(udpSocket) udpSocket->close(); }

    void sendStopAllCommand() override;
    void sendStopShovelCommand() override;

private slots:
    void readPendingDatagrams();
private:
    QUdpSocket *udpSocket;
};

#endif // UDPDATASOURCE_H
