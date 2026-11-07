#include "ui/MacroEditorDialog.h"
#include "engine/InputSender.h"
#include "hotkeys/Hotkey.h"
#include "ui/HotkeyEdit.h"
#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

namespace handi {

static QFrame* panel() {
    auto* f = new QFrame;
    f->setObjectName("panel");
    return f;
}

MacroEditorDialog::MacroEditorDialog(const Macro& m, QWidget* parent) : QDialog(parent), m_macro(m), m_actions(m.actions) {
    setWindowTitle(m.id.isEmpty() ? "Новый макрос" : "Редактор макроса");
    resize(820, 860);
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(22, 20, 22, 18);
    root->setSpacing(12);

    // --- Основные настройки ---
    auto* top = panel();
    auto* tg = new QFormLayout(top);
    tg->setContentsMargins(16, 14, 16, 14);
    tg->setHorizontalSpacing(14);
    m_name = new QLineEdit(m.name);
    m_name->setMaxLength(80);
    m_name->setPlaceholderText("Название макроса");
    tg->addRow("Название", m_name);

    auto* repRow = new QHBoxLayout;
    m_repeat = new QSpinBox;
    m_repeat->setRange(1, kMaxRepeat);
    m_repeat->setValue(m.repeatCount);
    m_repeat->setSuffix(" раз");
    m_infinite = new QCheckBox("Бесконечный повтор");
    m_infinite->setChecked(m.infinite);
    m_repeat->setEnabled(!m.infinite);
    connect(m_infinite, &QCheckBox::toggled, m_repeat, [this](bool on) { m_repeat->setEnabled(!on); });
    repRow->addWidget(m_repeat);
    repRow->addWidget(m_infinite);
    repRow->addStretch();
    tg->addRow("Повторы", repRow);

    auto* hkRow = new QHBoxLayout;
    m_hotkey = new HotkeyEdit(HotkeyEdit::Mode::Combo);
    if (auto h = Hotkey::fromString(m.hotkey)) m_hotkey->setHotkey(*h);
    auto* hkClear = new QPushButton("Очистить");
    connect(hkClear, &QPushButton::clicked, m_hotkey, &HotkeyEdit::clearValue);
    hkRow->addWidget(m_hotkey, 1);
    hkRow->addWidget(hkClear);
    tg->addRow("Клавиша запуска/остановки", hkRow);
    root->addWidget(top);

    // --- Быстрая запись: нажали клавишу, она добавилась ---
    auto* quickPanel = panel();
    auto* ql = new QHBoxLayout(quickPanel);
    ql->setContentsMargins(16, 12, 16, 12);
    ql->addWidget(new QLabel("Быстрая запись: нажмите клавишу"));
    auto* quick = new HotkeyEdit(HotkeyEdit::Mode::SingleKey);
    quick->setPlaceholderText("нажмите любую клавишу — она добавится");
    ql->addWidget(quick, 1);
    auto* qLmb = new QPushButton("ЛКМ");
    auto* qRmb = new QPushButton("ПКМ");
    auto* qWait = new QPushButton("Пауза 500 мс");
    for (auto* b : {qLmb, qRmb, qWait}) ql->addWidget(b);
    root->addWidget(quickPanel);
    connect(quick, &HotkeyEdit::valueChanged, this, [this, quick] {
        if (quick->vk() == 0 || m_actions.size() >= kMaxActions) return;
        Action a; a.type = ActionType::KeyPress; a.vk = quick->vk(); a.delayMs = m_delay->value();
        m_actions.append(a);
        quick->clearValue();
        refreshList(m_actions.size() - 1);
    });
    auto quickAdd = [this](ActionType t, MouseButton b, int delay) {
        if (m_actions.size() >= kMaxActions) return;
        Action a; a.type = t; a.button = b; a.delayMs = delay;
        m_actions.append(a);
        refreshList(m_actions.size() - 1);
    };
    connect(qLmb, &QPushButton::clicked, this, [this, quickAdd] { quickAdd(ActionType::MouseClick, MouseButton::Left, m_delay->value()); });
    connect(qRmb, &QPushButton::clicked, this, [this, quickAdd] { quickAdd(ActionType::MouseClick, MouseButton::Right, m_delay->value()); });
    connect(qWait, &QPushButton::clicked, this, [quickAdd] { quickAdd(ActionType::Delay, MouseButton::Left, 500); });

    // --- Визуальная последовательность ---
    auto* seqTitle = new QLabel("Последовательность действий");
    seqTitle->setStyleSheet("font-weight:600;font-size:15px;");
    root->addWidget(seqTitle);
    m_list = new QListWidget;
    m_list->setMinimumHeight(190);
    root->addWidget(m_list, 1);

    auto* listBtns = new QHBoxLayout;
    m_up = new QPushButton("↑ Выше");
    m_down = new QPushButton("↓ Ниже");
    m_dup = new QPushButton("Копировать");
    m_del = new QPushButton("Удалить");
    m_del->setProperty("danger", true);
    m_summary = new QLabel;
    m_summary->setObjectName("muted");
    for (auto* b : {m_up, m_down, m_dup, m_del}) listBtns->addWidget(b);
    listBtns->addStretch();
    listBtns->addWidget(m_summary);
    root->addLayout(listBtns);

    // --- Форма действия ---
    auto* form = panel();
    auto* fv = new QVBoxLayout(form);
    fv->setContentsMargins(16, 14, 16, 14);
    fv->setSpacing(10);

    auto* typeRow = new QHBoxLayout;
    typeRow->addWidget(new QLabel("Действие"));
    m_type = new QComboBox;
    m_type->addItem("Нажать клавишу", int(ActionType::KeyPress));
    m_type->addItem("Нажать и держать клавишу", int(ActionType::KeyDown));
    m_type->addItem("Отпустить клавишу", int(ActionType::KeyUp));
    m_type->addItem("Клик мышью", int(ActionType::MouseClick));
    m_type->addItem("Пауза", int(ActionType::Delay));
    m_type->addItem("Ждать подтверждения пользователя", int(ActionType::WaitConfirm));
    typeRow->addWidget(m_type, 1);
    fv->addLayout(typeRow);

    m_keyRow = new QWidget;
    auto* kl = new QHBoxLayout(m_keyRow);
    kl->setContentsMargins(0, 0, 0, 0);
    kl->addWidget(new QLabel("Клавиша"));
    m_key = new HotkeyEdit(HotkeyEdit::Mode::SingleKey);
    kl->addWidget(m_key, 1);
    fv->addWidget(m_keyRow);

    m_mouseRow = new QWidget;
    auto* ml = new QHBoxLayout(m_mouseRow);
    ml->setContentsMargins(0, 0, 0, 0);
    m_button = new QComboBox;
    m_button->addItem("Левая (ЛКМ)", int(MouseButton::Left));
    m_button->addItem("Правая (ПКМ)", int(MouseButton::Right));
    m_button->addItem("Средняя (СКМ)", int(MouseButton::Middle));
    m_useCoords = new QCheckBox("В точке экрана");
    m_x = new QSpinBox; m_x->setRange(-32768, 32768); m_x->setPrefix("X ");
    m_y = new QSpinBox; m_y->setRange(-32768, 32768); m_y->setPrefix("Y ");
    m_capture = new QPushButton("Захватить через 3 с");
    ml->addWidget(m_button);
    ml->addWidget(m_useCoords);
    ml->addWidget(m_x);
    ml->addWidget(m_y);
    ml->addWidget(m_capture);
    fv->addWidget(m_mouseRow);

    auto* dRow = new QHBoxLayout;
    dRow->addWidget(new QLabel("Задержка после действия"));
    m_delay = new QSpinBox;
    m_delay->setRange(0, kMaxDelayMs);
    m_delay->setSingleStep(10);
    m_delay->setSuffix(" мс");
    m_delay->setValue(50);
    dRow->addWidget(m_delay);
    dRow->addStretch();
    m_add = new QPushButton("Добавить в конец");
    m_add->setProperty("accent", true);
    m_apply = new QPushButton("Обновить выбранное");
    dRow->addWidget(m_apply);
    dRow->addWidget(m_add);
    fv->addLayout(dRow);
    root->addWidget(form);

    auto* bb = new QDialogButtonBox;
    auto* save = bb->addButton("Сохранить", QDialogButtonBox::AcceptRole);
    save->setProperty("accent", true);
    bb->addButton("Отмена", QDialogButtonBox::RejectRole);
    connect(bb, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(bb, &QDialogButtonBox::rejected, this, &QDialog::reject);
    root->addWidget(bb);

    // --- Связи ---
    connect(m_type, &QComboBox::currentIndexChanged, this, [this] { updateRows(); });
    connect(m_add, &QPushButton::clicked, this, &MacroEditorDialog::onAdd);
    connect(m_apply, &QPushButton::clicked, this, &MacroEditorDialog::onApply);
    connect(m_up, &QPushButton::clicked, this, [this] { onMove(-1); });
    connect(m_down, &QPushButton::clicked, this, [this] { onMove(1); });
    connect(m_dup, &QPushButton::clicked, this, &MacroEditorDialog::onDuplicate);
    connect(m_del, &QPushButton::clicked, this, &MacroEditorDialog::onDelete);
    connect(m_capture, &QPushButton::clicked, this, &MacroEditorDialog::onCapture);
    connect(m_list, &QListWidget::currentRowChanged, this, [this](int row) {
        if (row >= 0 && row < m_actions.size()) loadForm(m_actions[row]);
        updateButtons();
    });
    m_captureTimer.setInterval(1000);
    connect(&m_captureTimer, &QTimer::timeout, this, [this] {
        if (--m_countdown > 0) { m_capture->setText(QStringLiteral("Наведите курсор… %1").arg(m_countdown)); return; }
        m_captureTimer.stop();
        const QPoint p = input::cursorPos();
        m_x->setValue(p.x());
        m_y->setValue(p.y());
        m_useCoords->setChecked(true);
        m_capture->setText("Захватить через 3 с");
    });

    updateRows();
    refreshList(m_actions.isEmpty() ? -1 : 0);
}

void MacroEditorDialog::updateRows() {
    const auto t = ActionType(m_type->currentData().toInt());
    m_keyRow->setVisible(t == ActionType::KeyPress || t == ActionType::KeyDown || t == ActionType::KeyUp);
    m_mouseRow->setVisible(t == ActionType::MouseClick);
}

void MacroEditorDialog::updateButtons() {
    const int row = m_list->currentRow();
    const bool sel = row >= 0;
    m_apply->setEnabled(sel);
    m_up->setEnabled(sel && row > 0);
    m_down->setEnabled(sel && row < m_actions.size() - 1);
    m_dup->setEnabled(sel);
    m_del->setEnabled(sel);
}

void MacroEditorDialog::refreshList(int select) {
    QSignalBlocker block(m_list);
    m_list->clear();
    qint64 total = 0;
    for (int i = 0; i < m_actions.size(); ++i) {
        const Action& a = m_actions[i];
        total += a.delayMs + ((a.type == ActionType::KeyPress || a.type == ActionType::MouseClick) ? 25 : 0);
        auto* it = new QListWidgetItem(QStringLiteral("%1.   %2      ⏱ %3 мс").arg(i + 1).arg(a.describe()).arg(a.delayMs));
        if (a.type == ActionType::WaitConfirm) it->setForeground(QColor("#ff9a4d"));
        else if (a.type == ActionType::MouseClick) it->setForeground(QColor("#8ec5ff"));
        else if (a.type == ActionType::Delay) it->setForeground(QColor("#9a9aa2"));
        m_list->addItem(it);
    }
    m_summary->setText(QStringLiteral("Действий: %1 · один проход ≈ %2 с").arg(m_actions.size()).arg(total / 1000.0, 0, 'f', 2));
    if (select >= 0 && select < m_actions.size()) m_list->setCurrentRow(select);
    updateButtons();
}

bool MacroEditorDialog::formAction(Action& a) const {
    a = Action{};
    a.type = ActionType(m_type->currentData().toInt());
    a.delayMs = m_delay->value();
    if (a.type == ActionType::KeyPress || a.type == ActionType::KeyDown || a.type == ActionType::KeyUp) {
        if (m_key->vk() == 0) {
            QMessageBox::information(const_cast<MacroEditorDialog*>(this), "Клавиша", "Нажмите нужную клавишу в поле «Клавиша».");
            return false;
        }
        a.vk = m_key->vk();
    } else if (a.type == ActionType::MouseClick) {
        a.button = MouseButton(m_button->currentData().toInt());
        a.useCoords = m_useCoords->isChecked();
        a.x = m_x->value();
        a.y = m_y->value();
    }
    return true;
}

void MacroEditorDialog::loadForm(const Action& a) {
    m_type->setCurrentIndex(m_type->findData(int(a.type)));
    m_key->setVk(a.vk);
    m_button->setCurrentIndex(m_button->findData(int(a.button)));
    m_useCoords->setChecked(a.useCoords);
    m_x->setValue(a.x);
    m_y->setValue(a.y);
    m_delay->setValue(a.delayMs);
    updateRows();
}

void MacroEditorDialog::onAdd() {
    Action a;
    if (!formAction(a)) return;
    if (m_actions.size() >= kMaxActions) { QMessageBox::warning(this, "Лимит", "Достигнут максимум действий в макросе."); return; }
    m_actions.append(a);
    refreshList(m_actions.size() - 1);
}

void MacroEditorDialog::onApply() {
    const int row = m_list->currentRow();
    Action a;
    if (row < 0 || !formAction(a)) return;
    m_actions[row] = a;
    refreshList(row);
}

void MacroEditorDialog::onMove(int d) {
    const int row = m_list->currentRow(), to = row + d;
    if (row < 0 || to < 0 || to >= m_actions.size()) return;
    m_actions.swapItemsAt(row, to);
    refreshList(to);
}

void MacroEditorDialog::onDuplicate() {
    const int row = m_list->currentRow();
    if (row < 0 || m_actions.size() >= kMaxActions) return;
    m_actions.insert(row + 1, m_actions[row]);
    refreshList(row + 1);
}

void MacroEditorDialog::onDelete() {
    const int row = m_list->currentRow();
    if (row < 0) return;
    m_actions.removeAt(row);
    refreshList(qMin(row, int(m_actions.size()) - 1));
}

void MacroEditorDialog::onCapture() {
    if (m_captureTimer.isActive()) {
        m_captureTimer.stop();
        m_capture->setText("Захватить через 3 с");
        return;
    }
    m_countdown = 3;
    m_capture->setText("Наведите курсор… 3");
    m_captureTimer.start();
}

void MacroEditorDialog::accept() {
    const QString name = m_name->text().trimmed();
    if (name.isEmpty()) { QMessageBox::warning(this, "Название", "Введите название макроса."); return; }
    if (m_actions.isEmpty()) { QMessageBox::warning(this, "Пустой макрос", "Добавьте хотя бы одно действие."); return; }
    m_macro.name = name;
    m_macro.actions = m_actions;
    m_macro.repeatCount = m_repeat->value();
    m_macro.infinite = m_infinite->isChecked();
    m_macro.hotkey = m_hotkey->hotkey().toString();
    QDialog::accept();
}

}
