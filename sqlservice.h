#ifndef SQLSERVICE_H
#define SQLSERVICE_H

#include <QObject>
#include <QSqlDatabase>
#include <QVariantList>

class SQLService : public QObject {
    Q_OBJECT
public:
    explicit SQLService(QObject *parent = nullptr);
    ~SQLService();

    bool initialize();
    QVariantList loadAllAlarms();
    bool saveAlarm(const QString &timestamp, const QString &type, const QString &state, const QString &details);

private:
    QSqlDatabase m_db;
};

#endif // SQLSERVICE_H
