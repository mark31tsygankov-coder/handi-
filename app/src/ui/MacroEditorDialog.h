#pragma once
#include "macro/Macro.h"
#include <QDialog>
#include <QTimer>

class QCheckBox;
class QComboBox;
class QLabel;
class QLineEdit;
class QListWidget;
class QPushButton;
class QSpinBox;
class QWidget;

namespace handi {
class HotkeyEdit;

class MacroEditorDialog : public QDialog {
    Q_OBJECT
public:
    explicit MacroEditorDialog(const Macro& m, QWidget* parent = nullptr);
    Macro macro() const { return m_macro; }

protected:
    void accept() override;

private:
    void refreshList(int select = -1);
    bool formAction(Action& out) const;
    void loadForm(const Action& a);
    void updateRows();
    void updateButtons();
    void onAdd();
    void onApply();
    void onMove(int delta);
    void onDuplicate();
    void onDelete();
    void onCapture();

    Macro m_macro;
    QList<Action> m_actions;

    QLineEdit* m_name;
    QSpinBox* m_repeat;
    QCheckBox* m_infinite;
    HotkeyEdit* m_hotkey;
    QListWidget* m_list;
    QLabel* m_summary;

    QComboBox* m_type;
    QWidget* m_keyRow;
    HotkeyEdit* m_key;
    QWidget* m_mouseRow;
    QComboBox* m_button;
    QCheckBox* m_useCoords;
    QSpinBox* m_x;
    QSpinBox* m_y;
    QPushButton* m_capture;
    QSpinBox* m_delay;

    QPushButton *m_add, *m_apply, *m_up, *m_down, *m_dup, *m_del;
    QTimer m_captureTimer;
    int m_countdown = 0;
};

}
