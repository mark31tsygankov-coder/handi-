#pragma once
#include "macro/Macro.h"
#include "settings/AppSettings.h"
#include <QObject>
#include <optional>

namespace handi {

struct Profile {
    QString id;
    QString name;
    qint64 createdAt = 0;
};

class ProfileManager : public QObject {
    Q_OBJECT
public:
    ProfileManager(Database& db, AppSettings& settings, QObject* parent = nullptr);

    bool init(QString* error = nullptr);   // создаёт профиль «Скрайд» при первом запуске

    QList<Profile> profiles() const;
    QString activeProfileId() const;
    bool setActiveProfile(const QString& id);
    QString createProfile(const QString& name, QString* error = nullptr);
    bool renameProfile(const QString& id, const QString& name, QString* error = nullptr);
    bool deleteProfile(const QString& id, QString* error = nullptr);

    QList<Macro> macros(const QString& profileId) const;
    QList<Macro> allMacros() const;
    std::optional<Macro> macro(const QString& id) const;
    bool saveMacro(Macro& m, QString* error = nullptr);   // новый id/профиль назначаются автоматически
    bool deleteMacro(const QString& id);
    QString duplicateMacro(const QString& id);

    bool exportProfile(const QString& id, const QString& path, QString* error = nullptr) const;
    QString importProfile(const QString& path, QString* error = nullptr);

signals:
    void profilesChanged();
    void activeProfileChanged(const QString& id);
    void macrosChanged(const QString& profileId);

private:
    Macro rowToMacro(const QSqlQuery& q) const;
    Database& m_db;
    AppSettings& m_settings;
};

} // namespace handi
