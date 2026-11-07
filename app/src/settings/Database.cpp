#include "settings/Database.h"
#include <QDir>
#include <QFileInfo>
#include <QSqlError>
#include <QStandardPaths>
#include <QUuid>

namespace handi {

Database::Database(const QString& path) : m_path(path), m_conn(QUuid::createUuid().toString()) {}

Database::~Database() {
    if (m_db.isValid() && m_db.isOpen()) m_db.close();
    m_db = QSqlDatabase();
    QSqlDatabase::removeDatabase(m_conn);
}

QString Database::defaultPath() {
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/handi.db";
}

bool Database::exec(const QString& sql, const QVariantList& args, QString* error) {
    QSqlQuery q(m_db);
    q.prepare(sql);
    for (const QVariant& a : args) q.addBindValue(a);
    if (!q.exec()) {
        if (error) *error = q.lastError().text();
        return false;
    }
    return true;
}

bool Database::open(QString* error) {
    QDir().mkpath(QFileInfo(m_path).absolutePath());
    m_db = QSqlDatabase::addDatabase("QSQLITE", m_conn);
    m_db.setDatabaseName(m_path);
    if (!m_db.open()) {
        if (error) *error = m_db.lastError().text();
        return false;
    }
    const QStringList stmts = {
        "PRAGMA foreign_keys=ON",
        "PRAGMA journal_mode=WAL",
        "CREATE TABLE IF NOT EXISTS profiles ("
        " id TEXT PRIMARY KEY, name TEXT NOT NULL, created_at INTEGER NOT NULL)",
        "CREATE TABLE IF NOT EXISTS macros ("
        " id TEXT PRIMARY KEY, profile_id TEXT NOT NULL, name TEXT NOT NULL, data TEXT NOT NULL,"
        " created_at INTEGER NOT NULL, updated_at INTEGER NOT NULL,"
        " FOREIGN KEY(profile_id) REFERENCES profiles(id) ON DELETE CASCADE)",
        "CREATE INDEX IF NOT EXISTS idx_macros_profile ON macros(profile_id)",
        "CREATE TABLE IF NOT EXISTS settings (key TEXT PRIMARY KEY, value TEXT NOT NULL)",
    };
    for (const QString& s : stmts)
        if (!exec(s, {}, error)) return false;
    return true;
}

} // namespace handi
