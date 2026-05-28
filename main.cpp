#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QDir>

#include "configmanager.h"
#include "excavatorcontroller.h"
#include "udpdatasource.h"
#include "canbusdatasource.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    // Buscar el archivo config.json en el directorio donde corre la aplicación
    QString configPath = QCoreApplication::applicationDirPath() + "/config.json";

    // Cargar la configuración externa
    ConfigManager::loadConfig(configPath);

    // seteamos la DB
    SQLService* sqlService = new SQLService(&app);
    sqlService->initialize();

    ExcavatorDataSource* dataSource = nullptr;

    // Conmutar el comportamiento de la HMI según el JSON
    if (ConfigManager::mode == "production") {
        dataSource = new CanBusDataSource();
    } else {
        dataSource = new UdpDataSource();
    }

    dataSource->initialize();

    ExcavatorController excavatorController(dataSource, sqlService);

    dataSource->setParent(&excavatorController);

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("excavatorCtrl", &excavatorController);

    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed,
                     &app, []() { QCoreApplication::exit(-1); }, Qt::QueuedConnection);

    engine.loadFromModule("sample01", "Main");

    return app.exec();
}
