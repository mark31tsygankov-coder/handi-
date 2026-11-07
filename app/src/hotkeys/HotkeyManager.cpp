#include "hotkeys/HotkeyManager.h"
#include <QCoreApplication>
#include <windows.h>

namespace handi {

HotkeyManager::HotkeyManager(QObject* parent) : QObject(parent) {
    qApp->installNativeEventFilter(this);
}

HotkeyManager::~HotkeyManager() {
    unregisterAll();
    if (qApp) qApp->removeNativeEventFilter(this);
}

void HotkeyManager::unregisterAll() {
    for (auto it = m_cmdIds.cbegin(); it != m_cmdIds.cend(); ++it) UnregisterHotKey(nullptr, it.key());
    for (auto it = m_macroIds.cbegin(); it != m_macroIds.cend(); ++it) UnregisterHotKey(nullptr, it.key());
    m_cmdIds.clear();
    m_macroIds.clear();
    m_nextMacroId = 100;
}

QStringList HotkeyManager::apply(const QMap<Command, Hotkey>& global, const QMap<QString, Hotkey>& perMacro) {
    unregisterAll();
    m_lastGlobal = global;
    m_lastMacro = perMacro;
    QStringList errors;
    if (m_suspended) return errors;
    QList<Hotkey> used;

    auto tryRegister = [&](int id, const Hotkey& h, const QString& label) -> bool {
        if (!h.isValid()) return false;
        if (used.contains(h)) { errors << QStringLiteral("%1: сочетание %2 уже занято в HANDI").arg(label, h.toString()); return false; }
        if (!RegisterHotKey(nullptr, id, h.mods | MOD_NOREPEAT, h.vk)) {
            errors << QStringLiteral("%1: не удалось занять %2 (используется другой программой)").arg(label, h.toString());
            return false;
        }
        used.append(h);
        return true;
    };

    static const QMap<Command, QString> labels = {
        {Command::Run, "Запуск"}, {Command::Stop, "Остановка"}, {Command::Pause, "Пауза"},
        {Command::Resume, "Продолжение"}, {Command::ShowApp, "Открыть приложение"}};

    int id = 1;
    for (auto it = global.cbegin(); it != global.cend(); ++it, ++id)
        if (tryRegister(id, it.value(), labels.value(it.key()))) m_cmdIds.insert(id, it.key());

    for (auto it = perMacro.cbegin(); it != perMacro.cend(); ++it) {
        int mid = m_nextMacroId++;
        if (tryRegister(mid, it.value(), QStringLiteral("Макрос"))) m_macroIds.insert(mid, it.key());
    }
    return errors;
}

void HotkeyManager::setSuspended(bool suspended) {
    if (m_suspended == suspended) return;
    m_suspended = suspended;
    if (suspended) unregisterAll();
    else apply(m_lastGlobal, m_lastMacro);
}

bool HotkeyManager::nativeEventFilter(const QByteArray& eventType, void* message, qintptr*) {
    if (eventType != "windows_generic_MSG") return false;
    const MSG* msg = static_cast<const MSG*>(message);
    if (msg->message != WM_HOTKEY) return false;
    const int id = int(msg->wParam);
    if (m_cmdIds.contains(id)) { emit commandTriggered(m_cmdIds.value(id)); return true; }
    if (m_macroIds.contains(id)) { emit macroHotkeyTriggered(m_macroIds.value(id)); return true; }
    return false;
}

} // namespace handi
