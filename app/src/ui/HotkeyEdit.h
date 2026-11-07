#pragma once
#include "hotkeys/Hotkey.h"
#include <QLineEdit>
#include <functional>

namespace handi {

// Поле «нажмите сочетание»: записывает клавишу или сочетание по нажатию.
class HotkeyEdit : public QLineEdit {
    Q_OBJECT
public:
    enum class Mode { Combo, SingleKey };
    explicit HotkeyEdit(Mode mode = Mode::Combo, QWidget* parent = nullptr);

    Hotkey hotkey() const { return m_hk; }
    void setHotkey(const Hotkey& h);
    quint32 vk() const { return m_hk.vk; }
    void setVk(quint32 vk);
    void clearValue();

    // Пока поле в фокусе, глобальные клавиши снимаются, чтобы их можно было записать.
    static void setFocusHook(std::function<void(bool)> hook);

signals:
    void valueChanged();

protected:
    void keyPressEvent(QKeyEvent* e) override;
    void focusInEvent(QFocusEvent* e) override;
    void focusOutEvent(QFocusEvent* e) override;
    bool focusNextPrevChild(bool) override { return false; }

private:
    void refreshText();
    Mode m_mode;
    Hotkey m_hk;
};

}
