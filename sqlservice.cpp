#include "sqlservice.h"
#include <QCoreApplication>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>

SQLService::SQLService(QObject *parent) : QObject(parent) {}

SQLService::~SQLService() {
    if (m_db.isOpen()) {
        m_db.close();
    }
}

bool SQLService::initialize() {
    QString dbPath = QCoreApplication::applicationDirPath() + "/alarm_history.db";

    m_db = QSqlDatabase::addDatabase("QSQLITE");
    m_db.setDatabaseName(dbPath);

    if (!m_db.open()) {
        qWarning() << "❌ SQLService: Error abriendo base de datos:" << m_db.lastError().text();
        return false;
    }

    QSqlQuery query;
    QString createTableQuery = "CREATE TABLE IF NOT EXISTS alarms ("
                               "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                               "timestamp TEXT, "
                               "type TEXT, "
                               "state TEXT, "
                               "details TEXT)";

    if (!query.exec(createTableQuery)) {
        qWarning() << "❌ SQLService: Error creando tabla de alarmas:" << query.lastError().text();
        return false;
    }

    return true;
}

QVariantList SQLService::loadAllAlarms() {
    QVariantList history;
    QSqlQuery query("SELECT timestamp, type, state, details FROM alarms ORDER BY id DESC");

    while (query.next()) {
        QVariantMap event;
        event["timestamp"] = query.value(0).toString();
        event["type"] = query.value(1).toString();
        event["state"] = query.value(2).toString();
        event["details"] = query.value(3).toString();
        history.append(event);
    }

    return history;
}

bool SQLService::saveAlarm(const QString &timestamp, const QString &type, const QString &state, const QString &details) {
    QSqlQuery query;
    query.prepare("INSERT INTO alarms (timestamp, type, state, details) VALUES (:time, :type, :state, :details)");
    query.bindValue(":time", timestamp);
    query.bindValue(":type", type);
    query.bindValue(":state", state);
    query.bindValue(":details", details);

    if (!query.exec()) {
        qWarning() << "⚠️ SQLService: Error al guardar alarma:" << query.lastError().text();
        return false;
    }
    return true;
}
