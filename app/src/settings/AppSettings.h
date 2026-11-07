#pragma once
#include "settings/Database.h"
#include <QMap>

namespace handi {

class AppSettings {
public:
    explicit AppSettings(Database& db) : m_db(db) {}

    QString get(const QString& key, const QString& def = {}) const;
    bool set(const QString& key, const QString& value);
    bool getBool(const QString& key, bool def) const { return get(key, def ? "1" : "0") == "1"; }
    bool setBool(const QString& key, bool v) { return set(key, v ? "1" : "0"); }

    // Глобальные клавиши: run, stop, pause, resume, show
    static QMap<QString, QString> defaultHotkeys();
    QString hotkey(const QString& name) const;
    bool setHotkey(const QString& name, const QString& value);

private:
    Database& m_db;
};

} // namespace handi
