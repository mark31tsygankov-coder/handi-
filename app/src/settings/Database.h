#pragma once
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QString>
#include <QVariantList>

namespace handi {

// Локальное хранилище SQLite: профили, макросы, настройки.
class Database {
public:
    explicit Database(const QString& path);
    ~Database();
    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    static QString defaultPath();               // %APPDATA%/HANDI/handi.db
    bool open(QString* error = nullptr);
    QSqlDatabase& db() { return m_db; }
    bool exec(const QString& sql, const QVariantList& args = {}, QString* error = nullptr);

private:
    QString m_path;
    QString m_conn;
    QSqlDatabase m_db;
};

} // namespace handi
