#include "profiles/ProfileManager.h"
#include "hotkeys/Hotkey.h"
#include <QDateTime>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QSaveFile>
#include <QSqlError>

namespace handi {

static constexpr qint64 kMaxImportBytes = 5 * 1024 * 1024;
static constexpr int kMaxImportMacros = 500;

ProfileManager::ProfileManager(Database& db, AppSettings& settings, QObject* parent)
    : QObject(parent), m_db(db), m_settings(settings) {}

bool ProfileManager::init(QString* error) {
    if (profiles().isEmpty()) {
        const QString id = createProfile(QStringLiteral("Скрайд"), error);
        if (id.isEmpty()) return false;
    }
    const QString active = activeProfileId();
    if (active.isEmpty()) m_settings.set("activeProfile", profiles().first().id);
    return true;
}

QList<Profile> ProfileManager::profiles() const {
    QList<Profile> r;
    QSqlQuery q(m_db.db());
    if (q.exec("SELECT id,name,created_at FROM profiles ORDER BY created_at, rowid"))
        while (q.next()) r.append({q.value(0).toString(), q.value(1).toString(), q.value(2).toLongLong()});
    return r;
}

QString ProfileManager::activeProfileId() const {
    const QString id = m_settings.get("activeProfile");
    for (const Profile& p : profiles()) if (p.id == id) return id;
    const auto all = profiles();
    return all.isEmpty() ? QString() : all.first().id;
}

bool ProfileManager::setActiveProfile(const QString& id) {
    for (const Profile& p : profiles()) {
        if (p.id == id) {
            m_settings.set("activeProfile", id);
            emit activeProfileChanged(id);
            return true;
        }
    }
    return false;
}

QString ProfileManager::createProfile(const QString& nameIn, QString* error) {
    const QString name = nameIn.trimmed().left(60);
    if (name.isEmpty()) { if (error) *error = "Введите название профиля"; return {}; }
    const QString id = Macro::newId();
    if (!m_db.exec("INSERT INTO profiles(id,name,created_at) VALUES(?,?,?)",
                   {id, name, QDateTime::currentSecsSinceEpoch()}, error)) return {};
    emit profilesChanged();
    return id;
}

bool ProfileManager::renameProfile(const QString& id, const QString& nameIn, QString* error) {
    const QString name = nameIn.trimmed().left(60);
    if (name.isEmpty()) { if (error) *error = "Введите название профиля"; return false; }
    if (!m_db.exec("UPDATE profiles SET name=? WHERE id=?", {name, id}, error)) return false;
    emit profilesChanged();
    return true;
}

bool ProfileManager::deleteProfile(const QString& id, QString* error) {
    if (profiles().size() <= 1) { if (error) *error = "Нельзя удалить единственный профиль"; return false; }
    if (!m_db.exec("DELETE FROM profiles WHERE id=?", {id}, error)) return false;
    emit profilesChanged();
    if (m_settings.get("activeProfile") == id) setActiveProfile(profiles().first().id);
    return true;
}

Macro ProfileManager::rowToMacro(const QSqlQuery& q) const {
    Macro m = Macro::fromJson(QJsonDocument::fromJson(q.value(3).toString().toUtf8()).object());
    m.id = q.value(0).toString();
    m.profileId = q.value(1).toString();
    m.name = q.value(2).toString();
    return m;
}

QList<Macro> ProfileManager::macros(const QString& profileId) const {
    QList<Macro> r;
    QSqlQuery q(m_db.db());
    q.prepare("SELECT id,profile_id,name,data FROM macros WHERE profile_id=? ORDER BY created_at, rowid");
    q.addBindValue(profileId);
    if (q.exec()) while (q.next()) r.append(rowToMacro(q));
    return r;
}

QList<Macro> ProfileManager::allMacros() const {
    QList<Macro> r;
    QSqlQuery q(m_db.db());
    if (q.exec("SELECT id,profile_id,name,data FROM macros ORDER BY created_at, rowid"))
        while (q.next()) r.append(rowToMacro(q));
    return r;
}

std::optional<Macro> ProfileManager::macro(const QString& id) const {
    QSqlQuery q(m_db.db());
    q.prepare("SELECT id,profile_id,name,data FROM macros WHERE id=?");
    q.addBindValue(id);
    if (q.exec() && q.next()) return rowToMacro(q);
    return std::nullopt;
}

bool ProfileManager::saveMacro(Macro& m, QString* error) {
    m.name = m.name.trimmed().left(80);
    if (m.name.isEmpty()) { if (error) *error = "Введите название макроса"; return false; }
    if (m.actions.size() > kMaxActions) { if (error) *error = "Слишком много действий"; return false; }
    if (!m.hotkey.isEmpty() && !Hotkey::fromString(m.hotkey)) {
        if (error) *error = "Неверное сочетание клавиш: " + m.hotkey;
        return false;
    }
    if (m.id.isEmpty()) m.id = Macro::newId();
    if (m.profileId.isEmpty()) m.profileId = activeProfileId();
    const QString data = QString::fromUtf8(QJsonDocument(m.toJson()).toJson(QJsonDocument::Compact));
    const qint64 now = QDateTime::currentSecsSinceEpoch();
    if (!m_db.exec("INSERT INTO macros(id,profile_id,name,data,created_at,updated_at) VALUES(?,?,?,?,?,?) "
                   "ON CONFLICT(id) DO UPDATE SET name=excluded.name, data=excluded.data, updated_at=excluded.updated_at",
                   {m.id, m.profileId, m.name, data, now, now}, error)) return false;
    emit macrosChanged(m.profileId);
    return true;
}

bool ProfileManager::deleteMacro(const QString& id) {
    const auto m = macro(id);
    if (!m) return false;
    if (!m_db.exec("DELETE FROM macros WHERE id=?", {id})) return false;
    emit macrosChanged(m->profileId);
    return true;
}

QString ProfileManager::duplicateMacro(const QString& id) {
    auto m = macro(id);
    if (!m) return {};
    m->id.clear();
    m->name = (m->name + " (копия)").left(80);
    m->hotkey.clear();   // чтобы не было конфликта сочетаний
    return saveMacro(*m) ? m->id : QString();
}

bool ProfileManager::exportProfile(const QString& id, const QString& path, QString* error) const {
    QString name;
    for (const Profile& p : profiles()) if (p.id == id) name = p.name;
    if (name.isEmpty()) { if (error) *error = "Профиль не найден"; return false; }
    QJsonArray arr;
    for (const Macro& m : macros(id)) arr.append(m.toJson());
    const QJsonObject root{{"format", "handi-profile"}, {"version", 1}, {"name", name}, {"macros", arr}};
    QSaveFile f(path);
    if (!f.open(QIODevice::WriteOnly)) { if (error) *error = f.errorString(); return false; }
    f.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    if (!f.commit()) { if (error) *error = f.errorString(); return false; }
    return true;
}

QString ProfileManager::importProfile(const QString& path, QString* error) {
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) { if (error) *error = f.errorString(); return {}; }
    if (f.size() > kMaxImportBytes) { if (error) *error = "Файл слишком большой"; return {}; }
    QJsonParseError pe;
    const QJsonDocument doc = QJsonDocument::fromJson(f.readAll(), &pe);
    if (pe.error != QJsonParseError::NoError || !doc.isObject()) { if (error) *error = "Файл не является корректным JSON"; return {}; }
    const QJsonObject root = doc.object();
    if (root.value("format").toString() != "handi-profile" || root.value("version").toInt() != 1) {
        if (error) *error = "Это не профиль HANDI (формат или версия не поддерживается)";
        return {};
    }
    const QJsonArray arr = root.value("macros").toArray();
    if (arr.size() > kMaxImportMacros) { if (error) *error = "Слишком много макросов в файле"; return {}; }

    QString name = root.value("name").toString().trimmed().left(60);
    if (name.isEmpty()) name = "Импортированный профиль";
    QStringList existing;
    for (const Profile& p : profiles()) existing << p.name;
    QString unique = name;
    for (int i = 2; existing.contains(unique); ++i) unique = QStringLiteral("%1 (%2)").arg(name).arg(i);

    const QString pid = createProfile(unique, error);
    if (pid.isEmpty()) return {};
    for (const QJsonValue& v : arr) {
        if (!v.isObject()) continue;
        Macro m = Macro::fromJson(v.toObject());
        m.id.clear();
        m.profileId = pid;
        if (m.name.isEmpty()) m.name = "Макрос";
        if (!Hotkey::fromString(m.hotkey)) m.hotkey.clear();
        saveMacro(m);
    }
    return pid;
}

} // namespace handi
