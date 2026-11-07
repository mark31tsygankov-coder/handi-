#pragma once
#include "hotkeys/Hotkey.h"
#include <QAbstractNativeEventFilter>
#include <QHash>
#include <QMap>
#include <QObject>
#include <QStringList>

namespace handi {

class HotkeyManager : public QObject, public QAbstractNativeEventFilter {
    Q_OBJECT
public:
    enum class Command { Run, Stop, Pause, Resume, ShowApp };
    Q_ENUM(Command)

    explicit HotkeyManager(QObject* parent = nullptr);
    ~HotkeyManager() override;

    // Перерегистрирует все глобальные клавиши. Возвращает список ошибок (пусто = всё успешно).
    QStringList apply(const QMap<Command, Hotkey>& global, const QMap<QString, Hotkey>& perMacro);

    // Временно снимает все регистрации (пока пользователь записывает сочетание в поле ввода).
    void setSuspended(bool suspended);

    bool nativeEventFilter(const QByteArray& eventType, void* message, qintptr* result) override;

signals:
    void commandTriggered(handi::HotkeyManager::Command cmd);
    void macroHotkeyTriggered(const QString& macroId);

private:
    void unregisterAll();
    QHash<int, Command> m_cmdIds;
    QHash<int, QString> m_macroIds;
    int m_nextMacroId = 100;
    bool m_suspended = false;
    QMap<Command, Hotkey> m_lastGlobal;
    QMap<QString, Hotkey> m_lastMacro;
};

} // namespace handi
