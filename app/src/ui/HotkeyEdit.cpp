#include "ui/HotkeyEdit.h"
#include "macro/KeyNames.h"
#include <QKeyEvent>

namespace handi {

static std::function<void(bool)>& hook() {
    static std::function<void(bool)> h;
    return h;
}

void HotkeyEdit::setFocusHook(std::function<void(bool)> h) { hook() = std::move(h); }

HotkeyEdit::HotkeyEdit(Mode mode, QWidget* parent) : QLineEdit(parent), m_mode(mode) {
    setReadOnly(true);
    setCursor(Qt::PointingHandCursor);
    setPlaceholderText(mode == Mode::Combo ? "Нажмите сочетание клавиш…" : "Нажмите клавишу…");
}

void HotkeyEdit::refreshText() {
    setText(m_mode == Mode::Combo ? m_hk.toString() : keys::name(m_hk.vk));
}

void HotkeyEdit::setHotkey(const Hotkey& h) { m_hk = h; refreshText(); }
void HotkeyEdit::setVk(quint32 vk) { m_hk = Hotkey{0, vk}; refreshText(); }
void HotkeyEdit::clearValue() { m_hk = Hotkey{}; refreshText(); emit valueChanged(); }

void HotkeyEdit::keyPressEvent(QKeyEvent* e) {
    e->accept();
    const quint32 vk = e->nativeVirtualKey();
    if (vk == 0 || keys::name(vk).isEmpty()) return;
    if (m_mode == Mode::Combo) {
        if (vk == 0x10 || vk == 0x11 || vk == 0x12 || vk == 0x5B || vk == 0x5C) return;  // только модификатор
        unsigned mods = 0;
        const auto m = e->modifiers();
        if (m & Qt::ControlModifier) mods |= Hotkey::Ctrl;
        if (m & Qt::AltModifier) mods |= Hotkey::Alt;
        if (m & Qt::ShiftModifier) mods |= Hotkey::Shift;
        if (m & Qt::MetaModifier) mods |= Hotkey::Win;
        m_hk = Hotkey{mods, vk};
    } else {
        m_hk = Hotkey{0, vk};
    }
    refreshText();
    emit valueChanged();
}

void HotkeyEdit::focusInEvent(QFocusEvent* e) {
    QLineEdit::focusInEvent(e);
    if (hook()) hook()(true);
}

void HotkeyEdit::focusOutEvent(QFocusEvent* e) {
    QLineEdit::focusOutEvent(e);
    if (hook()) hook()(false);
}

}
