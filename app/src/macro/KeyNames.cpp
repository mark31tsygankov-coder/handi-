#include "macro/KeyNames.h"
#include <QHash>

namespace handi::keys {

const QList<QPair<quint32, QString>>& all() {
    static const QList<QPair<quint32, QString>> t = [] {
        QList<QPair<quint32, QString>> l;
        for (char c = 'A'; c <= 'Z'; ++c) l.append({quint32(c), QString(QLatin1Char(c))});
        for (char c = '0'; c <= '9'; ++c) l.append({quint32(c), QString(QLatin1Char(c))});
        for (int i = 1; i <= 24; ++i) l.append({quint32(0x6F + i), QStringLiteral("F%1").arg(i)});
        for (int i = 0; i <= 9; ++i) l.append({quint32(0x60 + i), QStringLiteral("Num%1").arg(i)});
        struct S { quint32 vk; const char* n; };
        static const S sp[] = {
            {0x08, "Backspace"}, {0x09, "Tab"}, {0x0D, "Enter"}, {0x10, "Shift"}, {0x11, "Ctrl"},
            {0x12, "Alt"}, {0x13, "Pause"}, {0x14, "CapsLock"}, {0x1B, "Esc"}, {0x20, "Space"},
            {0x21, "PageUp"}, {0x22, "PageDown"}, {0x23, "End"}, {0x24, "Home"}, {0x25, "Left"},
            {0x26, "Up"}, {0x27, "Right"}, {0x28, "Down"}, {0x2D, "Insert"}, {0x2E, "Delete"},
            {0x6A, "Num*"}, {0x6B, "Num+"}, {0x6D, "Num-"}, {0x6E, "Num."}, {0x6F, "Num/"},
            {0xBA, ";"}, {0xBB, "="}, {0xBC, ","}, {0xBD, "-"}, {0xBE, "."}, {0xBF, "/"},
            {0xC0, "`"}, {0xDB, "["}, {0xDC, "\\"}, {0xDD, "]"}, {0xDE, "'"},
        };
        for (const S& s : sp) l.append({s.vk, QString::fromUtf8(s.n)});
        return l;
    }();
    return t;
}

QString name(quint32 vk) {
    static const QHash<quint32, QString> m = [] {
        QHash<quint32, QString> h;
        for (const auto& p : all()) h.insert(p.first, p.second);
        return h;
    }();
    return m.value(vk);
}

quint32 fromName(const QString& n) {
    static const QHash<QString, quint32> m = [] {
        QHash<QString, quint32> h;
        for (const auto& p : all()) h.insert(p.second.toLower(), p.first);
        return h;
    }();
    return m.value(n.trimmed().toLower(), 0);
}

} // namespace handi::keys
