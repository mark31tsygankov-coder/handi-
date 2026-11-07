#pragma once
#include <QString>
#include <optional>

namespace handi {

// Модификаторы совпадают со значениями MOD_* из WinAPI.
struct Hotkey {
    enum Mod : unsigned { Alt = 1, Ctrl = 2, Shift = 4, Win = 8 };
    unsigned mods = 0;
    quint32 vk = 0;

    bool isValid() const { return vk != 0; }
    QString toString() const;                          // "Ctrl+Shift+F5"
    static std::optional<Hotkey> fromString(const QString& s);
    bool operator==(const Hotkey& o) const { return mods == o.mods && vk == o.vk; }
};

} // namespace handi
