#include "sqlservice.h"
#include <QCoreApplication>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>

/**
 * @brief Constructor de la clase SQLService.
 * @param parent Puntero al objeto QObject padre para delegar la gestión de memoria interna de Qt.
 */
SQLService::SQLService(QObject *parent) : QObject(parent) {}

/**
 * @brief Destructor de la clase SQLService.
 * Asegura el cierre ordenado y la desconexión del archivo de base de datos en el disco
 * antes de que el objeto sea destruido en la pila o memoria dinámica.
 */
SQLService::~SQLService() {
    if (m_db.isOpen()) {
        m_db.close();
    }
}

/**
 * @brief Configura e inicializa el motor de base de datos local SQLite.
 *
 * Este método define la ruta física del archivo `.db` en el directorio de ejecución de la HMI,
 * añade el driver nativo de SQLite, abre la conexión con el almacenamiento y genera
 * de forma automatizada la tabla estructurada de auditoría si esta no existía previamente.
 *
 * @return true si la base de datos se abrió y configuró correctamente; false si hay fallas
 *         de permisos en el disco o en la ejecución de la sintaxis DDL de SQL.
 */
bool SQLService::initialize() {
    // Definir de forma dinámica la ruta absoluta del archivo .db al lado del ejecutable final de la HMI
    QString dbPath = QCoreApplication::applicationDirPath() + "/alarm_history.db";

    // Registrar e instanciar el controlador embebido de SQLite provisto por el módulo Qt SQL
    m_db = QSqlDatabase::addDatabase("QSQLITE");
    m_db.setDatabaseName(dbPath);

    // Intentar realizar la apertura o creación del archivo físico en el disco duro
    if (!m_db.open()) {
        qWarning() << "❌ SQLService: Error abriendo base de datos:" << m_db.lastError().text();
        return false;
    }

    // Definir la consulta estructurada para la creación de la tabla de auditoría (Data Definition Language)
    QSqlQuery query;
    QString createTableQuery = "CREATE TABLE IF NOT EXISTS alarms ("
                               "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                               "timestamp TEXT, "
                               "type TEXT, "
                               "state TEXT, "
                               "details TEXT)";

    // Ejecutar la sentencia en el motor SQLite y validar errores sintácticos o de almacenamiento
    if (!query.exec(createTableQuery)) {
        qWarning() << "❌ SQLService: Error creando tabla de alarmas:" << query.lastError().text();
        return false;
    }

    return true;
}

/**
 * @brief Recupera la totalidad de los registros históricos almacenados en el disco duro.
 *
 * Ejecuta una consulta de selección estructurada extrayendo las alarmas ordenadas de forma
 * cronológica inversa (la más reciente al inicio), empaquetándolas en estructuras genéricas
 * legibles para el motor de Qt Quick.
 *
 * @return QVariantList Lista secuencial indexada de diccionarios (QVariantMap) que contiene
 *                      todas las filas de la base de datos para alimentar el ListView de QML.
 */
QVariantList SQLService::loadAllAlarms() {
    QVariantList history;

    // Ejecutar la consulta de extracción ordenando por ID de forma descendente (orden inverso)
    QSqlQuery query("SELECT timestamp, type, state, details FROM alarms ORDER BY id DESC");

    // Iterar de forma secuencial sobre el cursor de datos devuelto por SQLite
    while (query.next()) {
        QVariantMap event;
        // Mapear los índices de las columnas directamente a llaves alfanuméricas string
        event["timestamp"] = query.value(0).toString();
        event["type"] = query.value(1).toString();
        event["state"] = query.value(2).toString();
        event["details"] = query.value(3).toString();

        // Adjuntar el registro estructurado al contenedor de memoria RAM
        history.append(event);
    }

    return history;
}

/**
 * @brief Realiza la inserción permanente de una nueva alarma en la tabla de SQLite.
 *
 * Utiliza consultas preparadas parametrizadas (:placeholders) para garantizar la velocidad de
 * ejecución en hilos y blindar el sistema contra corrupciones de caracteres especiales en cadenas.
 *
 * @param timestamp Estampa de tiempo exacta formateada del evento (YYYY-MM-DD HH:MM:SS).
 * @param type Clasificación o ID de procedencia de la alarma (ej. "Inclinación", "Sistema").
 * @param state Estado lógico actual de la transición ("ACTIVADA", "RECONOCIDA", "NORMALIZADA").
 * @param details Información analítica complementaria de los floats de telemetría.
 * @return true si el registro fue insertado físicamente en la base de datos; false en caso de error.
 */
bool SQLService::saveAlarm(const QString &timestamp, const QString &type, const QString &state, const QString &details) {
    QSqlQuery query;

    // Preparar la plantilla de inserción para la compilación binaria previa en el motor SQL
    query.prepare("INSERT INTO alarms (timestamp, type, state, details) VALUES (:time, :type, :state, :details)");

    // Vincular de forma segura los valores de C++ a las variables internas de la consulta
    query.bindValue(":time", timestamp);
    query.bindValue(":type", type);
    query.bindValue(":state", state);
    query.bindValue(":details", details);

    // Confirmar la transacción en el archivo físico .db
    if (!query.exec()) {
        qWarning() << "⚠️ SQLService: Error al guardar alarma:" << query.lastError().text();
        return false;
    }
    return true;
}
