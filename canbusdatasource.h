#ifndef CANBUSDATASOURCE_H
#define CANBUSDATASOURCE_H

#include "excavatordatasource.h"
#include <QCanBusDevice>
#include <QCanBusFrame>

class CanBusDataSource : public ExcavatorDataSource {
    Q_OBJECT
public:
    explicit CanBusDataSource(QObject *parent = nullptr);
    ~CanBusDataSource() override;

    // Métodos obligatorios heredados de la interfaz
    bool initialize() override;
    void disconnectSource() override;

    // Métodos de envío bidireccional (Comandos de paro hacia la máquina)
    void sendStopAllCommand() override;
    void sendStopShovelCommand() override;

private slots:
    void readCanFrames();
    void handleDeviceError(QCanBusDevice::CanBusError error);

private:
    QCanBusDevice *m_canDevice;
};

#endif // CANBUSDATASOURCE_H
