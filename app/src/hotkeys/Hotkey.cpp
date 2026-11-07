#include "hotkeys/Hotkey.h"
#include "macro/KeyNames.h"
#include <QStringList>

namespace handi {

QString Hotkey::toString() const {
    if (!isValid()) return {};
    QStringList p;
    if (mods & Ctrl) p << "Ctrl";
    if (mods & Alt) p << "Alt";
    if (mods & Shift) p << "Shift";
    if (mods & Win) p << "Win";
    p << keys::name(vk);
    return p.join('+');
}

std::optional<Hotkey> Hotkey::fromString(const QString& s) {
    const QString t = s.trimmed();
    if (t.isEmpty()) return std::nullopt;
    Hotkey h;
    // "+" может быть самой клавишей только как "Num+", поэтому делим по '+' с учётом этого.
    QStringList parts = t.split('+');
    QStringList clean;
    for (const QString& p : parts) clean << p.trimmed();
    // восстановим "Num+" / "Num-" и т.п. если разбилось
    if (!clean.isEmpty() && clean.last().isEmpty() && clean.size() >= 2 && clean[clean.size() - 2].toLower() == "num") {
        clean.removeLast(); clean.last() = "Num+";
    }
    if (clean.isEmpty()) return std::nullopt;
    for (int i = 0; i < clean.size() - 1; ++i) {
        const QString m = clean[i].toLower();
        if (m == "ctrl" || m == "control") h.mods |= Ctrl;
        else if (m == "alt") h.mods |= Alt;
        else if (m == "shift") h.mods |= Shift;
        else if (m == "win") h.mods |= Win;
        else return std::nullopt;
    }
    h.vk = keys::fromName(clean.last());
    if (h.vk == 0 || h.vk == 0x10 || h.vk == 0x11 || h.vk == 0x12) return std::nullopt;
    return h;
}

} // namespace handi
