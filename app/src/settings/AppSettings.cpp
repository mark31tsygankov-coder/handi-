#include "settings/AppSettings.h"

namespace handi {

QString AppSettings::get(const QString& key, const QString& def) const {
    QSqlQuery q(m_db.db());
    q.prepare("SELECT value FROM settings WHERE key=?");
    q.addBindValue(key);
    if (q.exec() && q.next()) return q.value(0).toString();
    return def;
}

bool AppSettings::set(const QString& key, const QString& value) {
    return m_db.exec("INSERT INTO settings(key,value) VALUES(?,?) "
                     "ON CONFLICT(key) DO UPDATE SET value=excluded.value", {key, value});
}

QMap<QString, QString> AppSettings::defaultHotkeys() {
    return {{"run", "F6"}, {"stop", "F7"}, {"pause", "F8"}, {"resume", "F9"}, {"show", "F10"}};
}

QString AppSettings::hotkey(const QString& name) const {
    return get("hotkey." + name, defaultHotkeys().value(name));
}

bool AppSettings::setHotkey(const QString& name, const QString& value) {
    return set("hotkey." + name, value);
}

} // namespace handi
