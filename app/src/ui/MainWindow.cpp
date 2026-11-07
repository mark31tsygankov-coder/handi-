#include "ui/MainWindow.h"
#include "AppConfig.h"
#include "ui/Anim.h"
#include "ui/HotkeyEdit.h"
#include "ui/MacroEditorDialog.h"
#include <QApplication>
#include <QCheckBox>
#include <QCloseEvent>
#include <QComboBox>
#include <QDialog>
#include <QFileDialog>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPixmap>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QRegularExpression>
#include <QScrollArea>
#include <QSlider>
#include <QStackedWidget>
#include <QStyle>
#include <QSystemTrayIcon>
#include <QVBoxLayout>
#include <QWindow>

namespace handi {

static QString stateText(MacroEngine::State s) {
    switch (s) {
    case MacroEngine::State::Running: return "Работает";
    case MacroEngine::State::Paused: return "Пауза";
    case MacroEngine::State::WaitingConfirm: return "Ждёт подтверждения";
    default: return "Остановлен";
    }
}

static QPushButton* makeButton(const QString& text, const char* prop = nullptr) {
    auto* b = new QPushButton(text);
    b->setCursor(Qt::PointingHandCursor);
    if (prop) b->setProperty(prop, true);
    return b;
}

static QPushButton* iconButton(const QString& glyph, const QString& tip, const char* prop = nullptr) {
    auto* b = new QPushButton(glyph);
    b->setObjectName("ico");
    b->setToolTip(tip);
    b->setCursor(Qt::PointingHandCursor);
    b->setFocusPolicy(Qt::NoFocus);
    if (prop) b->setProperty(prop, true);
    return b;
}

static void repolish(QWidget* w) {
    w->style()->unpolish(w);
    w->style()->polish(w);
}

MainWindow::MainWindow(ProfileManager& pm, AppSettings& settings, MacroEngine& engine, HotkeyManager& hotkeys,
                       LicenseManager& license, QWidget* parent)
    : QMainWindow(parent), m_pm(pm), m_set(settings), m_engine(engine), m_hk(hotkeys), m_lic(license) {
    setWindowTitle("HANDI");
    setWindowIcon(QIcon(":/logo.png"));
    Qt::WindowFlags flags = Qt::Window | Qt::FramelessWindowHint;
    const bool pinned = m_set.getBool("pin", true);
    if (pinned) flags |= Qt::WindowStaysOnTopHint;
    setWindowFlags(flags);
    setAttribute(Qt::WA_TranslucentBackground);
    setMinimumSize(980, 640);
    resize(1080, 720);
    m_opacity = qBound(60, m_set.get("opacity", "100").toInt(), 100) / 100.0;
    setWindowOpacity(m_opacity);

    auto* outer = new QWidget;
    auto* ov = new QVBoxLayout(outer);
    ov->setContentsMargins(8, 8, 8, 8);
    auto* win = new QFrame;
    win->setObjectName("window");
    ov->addWidget(win);
    auto* v = new QVBoxLayout(win);
    v->setContentsMargins(0, 0, 0, 0);
    v->setSpacing(0);
    setCentralWidget(outer);

    // --- Верхняя панель ---
    m_topbar = new QFrame;
    m_topbar->setObjectName("topbar");
    m_topbar->setFixedHeight(68);
    m_topbar->installEventFilter(this);
    auto* t = new QHBoxLayout(m_topbar);
    t->setContentsMargins(18, 0, 14, 0);
    t->setSpacing(8);
    auto* logo = new QLabel;
    logo->setPixmap(QPixmap(":/logo.png").scaled(38, 38, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    t->addWidget(logo);
    auto* brand = new QLabel("HANDI");
    brand->setObjectName("brand");
    t->addWidget(brand);
    t->addSpacing(10);
    const QStringList names = {"Макросы", "Профили", "Настройки"};
    for (int i = 0; i < names.size(); ++i) {
        auto* b = new QPushButton(names[i]);
        b->setObjectName("navpill");
        b->setCheckable(true);
        b->setAutoExclusive(true);
        b->setCursor(Qt::PointingHandCursor);
        b->setFocusPolicy(Qt::NoFocus);
        connect(b, &QPushButton::clicked, this, [this, i] { setPage(i); });
        m_nav << b;
        t->addWidget(b);
    }
    t->addSpacing(8);
    m_search = new QLineEdit;
    m_search->setObjectName("search");
    m_search->setPlaceholderText("Поиск по макросам…");
    m_search->setClearButtonEnabled(true);
    t->addWidget(m_search, 1);
    m_pill = new QLabel("Остановлен");
    m_pill->setObjectName("pill");
    t->addWidget(m_pill);
    m_pin = iconButton("📌", "Поверх всех окон");
    m_pin->setCheckable(true);
    m_pin->setChecked(pinned);
    t->addWidget(m_pin);
    auto* bMin = iconButton("—", "Свернуть");
    auto* bClose = iconButton("✕", "Закрыть");
    t->addWidget(bMin);
    t->addWidget(bClose);
    connect(bMin, &QPushButton::clicked, this, &QWidget::showMinimized);
    connect(bClose, &QPushButton::clicked, this, &QWidget::close);
    connect(m_pin, &QPushButton::toggled, this, [this](bool on) {
        m_set.setBool("pin", on);
        const bool vis = isVisible();
        setWindowFlag(Qt::WindowStaysOnTopHint, on);
        if (vis) show();
    });
    connect(m_search, &QLineEdit::textChanged, this, [this] { reloadMacros(); });
    v->addWidget(m_topbar);

    // --- Баннер ожидания подтверждения ---
    m_banner = new QFrame;
    m_banner->setObjectName("banner");
    m_banner->hide();
    auto* bl = new QHBoxLayout(m_banner);
    bl->setContentsMargins(16, 10, 16, 10);
    auto* bt = new QLabel("Макрос ждёт вашего подтверждения (например, проверка «Вы человек?» в игре). "
                          "Выполните её вручную, затем нажмите «Продолжить» или клавишу продолжения.");
    bt->setWordWrap(true);
    bl->addWidget(bt, 1);
    auto* bc = makeButton("Продолжить", "accent");
    connect(bc, &QPushButton::clicked, &m_engine, &MacroEngine::resume);
    bl->addWidget(bc);
    auto* bannerWrap = new QVBoxLayout;
    bannerWrap->setContentsMargins(24, 14, 24, 0);
    bannerWrap->addWidget(m_banner);
    v->addLayout(bannerWrap);

    m_stack = new QStackedWidget;
    m_stack->addWidget(buildMacrosPage());
    m_stack->addWidget(buildProfilesPage());
    m_stack->addWidget(buildSettingsPage());
    v->addWidget(m_stack, 1);

    // --- Трей ---
    m_tray = new QSystemTrayIcon(QIcon(":/logo.png"), this);
    auto* menu = new QMenu(this);
    menu->addAction("Открыть HANDI", this, &MainWindow::showAndRaise);
    menu->addAction("Остановить макрос", &m_engine, &MacroEngine::stop);
    menu->addSeparator();
    menu->addAction("Выход", qApp, &QApplication::quit);
    m_tray->setContextMenu(menu);
    m_tray->setToolTip("HANDI");
    connect(m_tray, &QSystemTrayIcon::activated, this, [this](QSystemTrayIcon::ActivationReason r) {
        if (r == QSystemTrayIcon::Trigger || r == QSystemTrayIcon::DoubleClick) showAndRaise();
    });
    m_tray->show();

    // --- Связи с ядром ---
    connect(&m_engine, &MacroEngine::stateChanged, this, &MainWindow::onEngineState);
    connect(&m_engine, &MacroEngine::progress, this, &MainWindow::onProgress);
    connect(&m_hk, &HotkeyManager::commandTriggered, this, &MainWindow::onCommand);
    connect(&m_hk, &HotkeyManager::macroHotkeyTriggered, this, &MainWindow::onMacroHotkey);
    connect(&m_pm, &ProfileManager::profilesChanged, this, [this] { reloadProfiles(); reloadProfileList(); });
    connect(&m_pm, &ProfileManager::activeProfileChanged, this, [this] { reloadProfiles(); reloadMacros(); reloadProfileList(); });
    connect(&m_pm, &ProfileManager::macrosChanged, this, [this](const QString& pid) {
        if (pid == m_pm.activeProfileId()) reloadMacros();
        reloadProfileList();
    });
    m_startTimer.setInterval(1000);
    connect(&m_startTimer, &QTimer::timeout, this, [this] {
        if (--m_countdown > 0) return;
        m_startTimer.stop();
        startMacro(m_pending);
    });
    HotkeyEdit::setFocusHook([this](bool focused) { m_hk.setSuspended(focused); });

    reloadProfiles();
    reloadMacros();
    reloadProfileList();
    m_nav[0]->setChecked(true);
    updateButtons();
}

void MainWindow::showAndRaise() {
    setWindowOpacity(0.0);
    showNormal();
    raise();
    activateWindow();
    auto* a = new QPropertyAnimation(this, "windowOpacity", this);
    a->setDuration(220);
    a->setStartValue(0.0);
    a->setEndValue(m_opacity);
    a->setEasingCurve(QEasingCurve::OutCubic);
    a->start(QAbstractAnimation::DeleteWhenStopped);
}

void MainWindow::toggleVisible() {
    if (isVisible() && !isMinimized()) hide();
    else showAndRaise();
}

void MainWindow::closeEvent(QCloseEvent* e) {
    if (m_set.getBool("tray.close", false) && m_tray->isVisible()) {
        hide();
        e->ignore();
        return;
    }
    cancelPending();
    m_engine.stop();
    e->accept();
    qApp->quit();
}

void MainWindow::setPage(int i) {
    m_nav[i]->setChecked(true);
    m_stack->setCurrentIndex(i);
    anim::fadeIn(m_stack->currentWidget());
}

bool MainWindow::eventFilter(QObject* obj, QEvent* e) {
    if (obj == m_topbar && e->type() == QEvent::MouseButtonPress) {
        auto* me = static_cast<QMouseEvent*>(e);
        if (me->button() == Qt::LeftButton && windowHandle()) {
            windowHandle()->startSystemMove();   // перетаскивание окна за верхнюю панель
            return true;
        }
    }
    if (e->type() == QEvent::MouseButtonRelease) {
        auto* w = qobject_cast<QWidget*>(obj);
        auto* me = static_cast<QMouseEvent*>(e);
        if (w && me->button() == Qt::LeftButton && w->rect().contains(me->position().toPoint())) {
            const QString act = w->property("tileAction").toString();
            if (!act.isEmpty()) {
                if (act == "new") createMacro();
                else if (act == "profiles") setPage(1);
                else setPage(2);
                return true;
            }
            const QString id = w->property("macroId").toString();
            if (!id.isEmpty()) selectMacro(id);
        }
    }
    return QMainWindow::eventFilter(obj, e);
}

// ------------------------------------------------------------------ страница «Макросы»
QWidget* MainWindow::buildMacrosPage() {
    auto* page = new QWidget;
    auto* v = new QVBoxLayout(page);
    v->setContentsMargins(24, 18, 24, 18);
    v->setSpacing(14);

    // плитки быстрых действий
    auto makeTile = [this](const QString& action, const QString& glyph, const QString& title, const QString& sub, bool accent) {
        auto* f = new QFrame;
        f->setObjectName("tile");
        f->setProperty("tileAction", action);
        f->setProperty("accentTile", accent);
        f->setCursor(Qt::PointingHandCursor);
        f->installEventFilter(this);
        auto* l = new QHBoxLayout(f);
        l->setContentsMargins(14, 12, 16, 12);
        l->setSpacing(12);
        auto* ic = new QLabel(glyph);
        ic->setObjectName(accent ? "tileIconAccent" : "tileIcon");
        ic->setFixedSize(42, 42);
        ic->setAlignment(Qt::AlignCenter);
        auto* col = new QVBoxLayout;
        col->setSpacing(0);
        auto* tt = new QLabel(title);
        tt->setObjectName("tileTitle");
        auto* ss = new QLabel(sub);
        ss->setObjectName("muted");
        col->addWidget(tt);
        col->addWidget(ss);
        l->addWidget(ic);
        l->addLayout(col, 1);
        auto* arrow = new QLabel("›");
        arrow->setObjectName("muted");
        l->addWidget(arrow);
        return f;
    };
    auto* tiles = new QHBoxLayout;
    tiles->setSpacing(12);
    tiles->addWidget(makeTile("new", "＋", "Создать макрос", "клавиши, клики, паузы", true), 3);
    tiles->addWidget(makeTile("profiles", "☰", "Профили", "наборы макросов", false), 2);
    tiles->addWidget(makeTile("settings", "⚙", "Настройки", "клавиши и вид", false), 2);
    v->addLayout(tiles);

    // управление запуском
    auto* tb = new QHBoxLayout;
    tb->setSpacing(8);
    m_btnRun = makeButton("▶  Запустить выбранный", "accent");
    m_btnStop = makeButton("■  Стоп");
    m_btnPause = makeButton("❚❚  Пауза");
    m_btnResume = makeButton("Продолжить");
    for (auto* b : {m_btnRun, m_btnStop, m_btnPause, m_btnResume}) tb->addWidget(b);
    tb->addStretch();
    auto* pl = new QLabel("Профиль");
    pl->setObjectName("muted");
    tb->addWidget(pl);
    m_profileCombo = new QComboBox;
    m_profileCombo->setMinimumWidth(180);
    tb->addWidget(m_profileCombo);
    v->addLayout(tb);
    connect(m_profileCombo, &QComboBox::currentIndexChanged, this, [this](int) {
        if (m_loading) return;
        const QString id = m_profileCombo->currentData().toString();
        if (!id.isEmpty() && id != m_pm.activeProfileId()) m_pm.setActiveProfile(id);
    });

    // карточки макросов
    m_scroll = new QScrollArea;
    m_scroll->setWidgetResizable(true);
    m_scroll->setFrameShape(QFrame::NoFrame);
    m_scroll->viewport()->setAutoFillBackground(false);
    m_cardsHost = new QWidget;
    m_hostLayout = new QVBoxLayout(m_cardsHost);
    m_hostLayout->setContentsMargins(0, 0, 4, 0);
    m_hostLayout->addStretch(1);
    m_scroll->setWidget(m_cardsHost);
    v->addWidget(m_scroll, 1);

    // недавние
    m_recentTitle = new QLabel("Недавно запускали");
    m_recentTitle->setObjectName("section");
    v->addWidget(m_recentTitle);
    m_recentLayout = new QHBoxLayout;
    m_recentLayout->setSpacing(10);
    v->addLayout(m_recentLayout);

    m_info = new QLabel;
    m_info->setObjectName("muted");
    m_info->setWordWrap(true);
    v->addWidget(m_info);
    m_hkWarn = new QLabel;
    m_hkWarn->setObjectName("warn");
    m_hkWarn->setWordWrap(true);
    m_hkWarn->hide();
    v->addWidget(m_hkWarn);

    connect(m_btnRun, &QPushButton::clicked, this, [this] { runSelected(false); });
    connect(m_btnStop, &QPushButton::clicked, this, [this] { cancelPending(); m_engine.stop(); });
    connect(m_btnPause, &QPushButton::clicked, &m_engine, &MacroEngine::pause);
    connect(m_btnResume, &QPushButton::clicked, &m_engine, &MacroEngine::resume);
    return page;
}

QFrame* MainWindow::buildCard(const Macro& m) {
    auto* card = new QFrame;
    card->setObjectName("macroCard");
    card->setProperty("macroId", m.id);
    card->setProperty("selected", m.id == m_selectedId);
    card->setCursor(Qt::PointingHandCursor);
    card->installEventFilter(this);
    auto* cl = new QVBoxLayout(card);
    cl->setContentsMargins(18, 16, 18, 14);
    cl->setSpacing(8);

    auto* top = new QHBoxLayout;
    auto* name = new QLabel(m.name);
    name->setObjectName("cardTitle");
    top->addWidget(name, 1);
    auto* st = new QLabel("Остановлен");
    st->setObjectName("state");
    top->addWidget(st);
    cl->addLayout(top);
    m_stateLabels.insert(m.id, st);

    const QString rep = m.infinite ? QStringLiteral("∞ повтор") : QStringLiteral("×%1").arg(m.repeatCount);
    auto* info = new QLabel(QStringLiteral("%1 действий · %2").arg(m.actions.size()).arg(rep));
    info->setObjectName("muted");
    cl->addWidget(info);

    auto* bottom = new QHBoxLayout;
    bottom->setSpacing(6);
    const QString id = m.id;
    auto* key = new QPushButton(m.hotkey.isEmpty() ? QStringLiteral("＋ Выбрать клавишу") : m.hotkey);
    key->setObjectName("keychip");
    key->setCursor(Qt::PointingHandCursor);
    key->setFocusPolicy(Qt::NoFocus);
    key->setToolTip("Клавиша запуска и остановки этого макроса");
    connect(key, &QPushButton::clicked, this, [this, id] { assignHotkey(id); });
    bottom->addWidget(key);
    bottom->addStretch(1);
    auto* run = iconButton("▶", "Запустить", "accent");
    auto* edit = iconButton("✎", "Редактировать");
    auto* copy = iconButton("⧉", "Копировать");
    auto* del = iconButton("🗑", "Удалить", "danger");
    for (auto* b : {run, edit, copy, del}) bottom->addWidget(b);
    cl->addLayout(bottom);
    connect(run, &QPushButton::clicked, this, [this, id] { runMacroById(id, false); });
    connect(edit, &QPushButton::clicked, this, [this, id] { editMacroById(id); });
    connect(copy, &QPushButton::clicked, this, [this, id] { duplicateMacroById(id); });
    connect(del, &QPushButton::clicked, this, [this, id] { deleteMacroById(id); });
    m_cards.insert(m.id, card);
    return card;
}

void MainWindow::reloadProfiles() {
    m_loading = true;
    m_profileCombo->clear();
    const QString active = m_pm.activeProfileId();
    int idx = 0, i = 0;
    for (const Profile& p : m_pm.profiles()) {
        m_profileCombo->addItem(p.name, p.id);
        if (p.id == active) idx = i;
        ++i;
    }
    m_profileCombo->setCurrentIndex(idx);
    m_loading = false;
}

void MainWindow::reloadMacros() {
    const auto macros = m_pm.macros(m_pm.activeProfileId());
    const QString q = m_search ? m_search->text().trimmed() : QString();
    bool found = false;
    for (const Macro& m : macros) if (m.id == m_selectedId) found = true;
    if (!found) m_selectedId = macros.isEmpty() ? QString() : macros.first().id;

    if (m_gridHost) {
        m_hostLayout->removeWidget(m_gridHost);
        m_gridHost->deleteLater();
    }
    m_stateLabels.clear();
    m_cards.clear();
    m_gridHost = new QWidget;
    auto* grid = new QGridLayout(m_gridHost);
    grid->setContentsMargins(0, 0, 0, 0);
    grid->setSpacing(12);
    grid->setColumnStretch(0, 1);
    grid->setColumnStretch(1, 1);
    int n = 0;
    for (const Macro& m : macros) {
        if (!q.isEmpty() && !m.name.contains(q, Qt::CaseInsensitive)) continue;
        grid->addWidget(buildCard(m), n / 2, n % 2);
        ++n;
    }
    if (n == 0) {
        auto* empty = new QLabel(macros.isEmpty() ? "Здесь пока пусто. Нажмите «Создать макрос», добавьте клавиши и выберите клавишу запуска."
                                                  : "Ничего не найдено.");
        empty->setObjectName("muted");
        empty->setWordWrap(true);
        grid->addWidget(empty, 0, 0, 1, 2);
    }
    m_hostLayout->insertWidget(0, m_gridHost);
    updateStates();
    updateButtons();
    reloadRecent();
    applyHotkeys();
    if (m_state == MacroEngine::State::Idle) m_info->clear();
}

void MainWindow::reloadRecent() {
    if (!m_recentLayout) return;
    while (auto* it = m_recentLayout->takeAt(0)) {
        if (auto* w = it->widget()) w->deleteLater();
        delete it;
    }
    const QStringList ids = m_set.get("recent").split(',', Qt::SkipEmptyParts);
    int shown = 0;
    for (const QString& id : ids) {
        const auto m = m_pm.macro(id);
        if (!m || m->profileId != m_pm.activeProfileId()) continue;
        auto* f = new QFrame;
        f->setObjectName("recent");
        f->setProperty("macroId", id);
        f->setCursor(Qt::PointingHandCursor);
        f->installEventFilter(this);
        auto* l = new QVBoxLayout(f);
        l->setContentsMargins(14, 10, 14, 10);
        l->setSpacing(0);
        auto* n = new QLabel(m->name);
        n->setStyleSheet("font-weight:600;");
        auto* k = new QLabel(m->hotkey.isEmpty() ? QStringLiteral("без клавиши") : m->hotkey);
        k->setObjectName("muted");
        l->addWidget(n);
        l->addWidget(k);
        m_recentLayout->addWidget(f);
        if (++shown >= 5) break;
    }
    m_recentLayout->addStretch(1);
    m_recentTitle->setVisible(shown > 0);
}

void MainWindow::pushRecent(const QString& id) {
    QStringList ids = m_set.get("recent").split(',', Qt::SkipEmptyParts);
    ids.removeAll(id);
    ids.prepend(id);
    while (ids.size() > 8) ids.removeLast();
    m_set.set("recent", ids.join(','));
    reloadRecent();
}

void MainWindow::selectMacro(const QString& id) {
    m_selectedId = id;
    for (auto it = m_cards.cbegin(); it != m_cards.cend(); ++it) {
        it.value()->setProperty("selected", it.key() == id);
        repolish(it.value());
    }
}

void MainWindow::updateStates() {
    for (auto it = m_stateLabels.cbegin(); it != m_stateLabels.cend(); ++it) {
        const bool mine = m_state != MacroEngine::State::Idle && it.key() == m_runningId;
        it.value()->setText(mine ? stateText(m_state) : "Остановлен");
        it.value()->setStyleSheet(mine ? "font-weight:700;color:#ff7a1a;" : "");
    }
    m_pill->setText(stateText(m_state));
    m_pill->setStyleSheet(m_state == MacroEngine::State::Idle ? "" : "color:#ff7a1a;border-color:#ff6a00;");
}

void MainWindow::updateButtons() {
    const bool idle = m_state == MacroEngine::State::Idle && !m_startTimer.isActive();
    m_btnRun->setEnabled(idle && !m_selectedId.isEmpty());
    m_btnStop->setEnabled(!idle);
    m_btnPause->setEnabled(m_state == MacroEngine::State::Running);
    m_btnResume->setEnabled(m_state == MacroEngine::State::Paused || m_state == MacroEngine::State::WaitingConfirm);
    m_profileCombo->setEnabled(idle);
}

// ------------------------------------------------------------------ действия с макросами
void MainWindow::createMacro() {
    Macro m;
    m.profileId = m_pm.activeProfileId();
    m.name = "Новый макрос";
    MacroEditorDialog dlg(m, this);
    if (dlg.exec() != QDialog::Accepted) return;
    Macro saved = dlg.macro();
    QString err;
    if (!m_pm.saveMacro(saved, &err)) QMessageBox::warning(this, "Ошибка сохранения", err);
    else m_selectedId = saved.id;
}

void MainWindow::editMacroById(const QString& id) {
    const auto m = m_pm.macro(id);
    if (!m || m_state != MacroEngine::State::Idle) { m_info->setText("Остановите макрос, чтобы его изменить."); return; }
    MacroEditorDialog dlg(*m, this);
    if (dlg.exec() != QDialog::Accepted) return;
    Macro saved = dlg.macro();
    QString err;
    if (!m_pm.saveMacro(saved, &err)) QMessageBox::warning(this, "Ошибка сохранения", err);
}

void MainWindow::deleteMacroById(const QString& id) {
    const auto m = m_pm.macro(id);
    if (!m || m_state != MacroEngine::State::Idle) return;
    if (QMessageBox::question(this, "Удаление", QStringLiteral("Удалить макрос «%1»?").arg(m->name)) != QMessageBox::Yes) return;
    m_pm.deleteMacro(id);
}

void MainWindow::duplicateMacroById(const QString& id) {
    if (m_pm.duplicateMacro(id).isEmpty()) m_info->setText("Не удалось скопировать макрос.");
}

void MainWindow::assignHotkey(const QString& id) {
    auto m = m_pm.macro(id);
    if (!m) return;
    QDialog dlg(this);
    dlg.setWindowTitle("Клавиша макроса");
    dlg.setMinimumWidth(420);
    auto* v = new QVBoxLayout(&dlg);
    v->setContentsMargins(22, 20, 22, 18);
    v->setSpacing(12);
    auto* title = new QLabel(QStringLiteral("Нажмите сочетание клавиш для «%1»").arg(m->name));
    title->setWordWrap(true);
    title->setStyleSheet("font-size:16px;font-weight:600;");
    v->addWidget(title);
    auto* ed = new HotkeyEdit(HotkeyEdit::Mode::Combo);
    if (auto h = Hotkey::fromString(m->hotkey)) ed->setHotkey(*h);
    ed->setMinimumHeight(48);
    ed->setAlignment(Qt::AlignCenter);
    v->addWidget(ed);
    auto* hint = new QLabel("Эта клавиша запускает макрос, повторное нажатие останавливает его. Можно F-клавиши, цифры, Num-клавиши и сочетания с Ctrl/Alt/Shift.");
    hint->setObjectName("muted");
    hint->setWordWrap(true);
    v->addWidget(hint);
    auto* row = new QHBoxLayout;
    auto* clear = makeButton("Очистить");
    auto* cancel = makeButton("Отмена");
    auto* ok = makeButton("Сохранить", "accent");
    row->addWidget(clear);
    row->addStretch();
    row->addWidget(cancel);
    row->addWidget(ok);
    v->addLayout(row);
    connect(clear, &QPushButton::clicked, ed, &HotkeyEdit::clearValue);
    connect(cancel, &QPushButton::clicked, &dlg, &QDialog::reject);
    connect(ok, &QPushButton::clicked, &dlg, &QDialog::accept);
    ed->setFocus();
    if (dlg.exec() != QDialog::Accepted) return;
    m->hotkey = ed->hotkey().toString();
    QString err;
    if (!m_pm.saveMacro(*m, &err)) QMessageBox::warning(this, "Ошибка", err);
}

void MainWindow::runMacroById(const QString& id, bool immediate) {
    if (m_state != MacroEngine::State::Idle || m_startTimer.isActive()) {
        m_info->setText("Сначала остановите текущий макрос.");
        return;
    }
    const auto m = m_pm.macro(id);
    if (!m) { m_info->setText("Сначала создайте или выберите макрос."); return; }
    selectMacro(id);
    if (immediate) { startMacro(*m); return; }
    // Запуск кнопкой: 3 секунды, чтобы переключиться в игру, иначе нажатия попадут в окно HANDI.
    m_pending = *m;
    m_countdown = 3;
    m_info->setText("Запуск через 3 секунды — переключитесь в игру. Остановка: клавиша остановки.");
    updateButtons();
    showMinimized();
    m_startTimer.start();
}

void MainWindow::runSelected(bool immediate) {
    QString id = m_selectedId;
    if (id.isEmpty()) {
        const auto l = m_pm.macros(m_pm.activeProfileId());
        if (!l.isEmpty()) id = l.first().id;
    }
    runMacroById(id, immediate);
}

void MainWindow::startMacro(const Macro& m) {
    if (!m_engine.startMacro(m)) m_info->setText("Не удалось запустить: макрос пуст или уже выполняется другой.");
    updateButtons();
}

void MainWindow::cancelPending() {
    if (m_startTimer.isActive()) {
        m_startTimer.stop();
        m_info->setText("Запуск отменён.");
        updateButtons();
    }
}

void MainWindow::onCommand(HotkeyManager::Command cmd) {
    switch (cmd) {
    case HotkeyManager::Command::Run: runSelected(true); break;
    case HotkeyManager::Command::Stop: cancelPending(); m_engine.stop(); break;
    case HotkeyManager::Command::Pause: m_engine.pause(); break;
    case HotkeyManager::Command::Resume: m_engine.resume(); break;
    case HotkeyManager::Command::ShowApp: toggleVisible(); break;
    }
}

void MainWindow::onMacroHotkey(const QString& id) {
    if (m_engine.state() != MacroEngine::State::Idle) {
        if (m_engine.currentMacroId() == id) m_engine.stop();   // повторное нажатие останавливает
        return;
    }
    if (const auto m = m_pm.macro(id)) startMacro(*m);
}

void MainWindow::onEngineState(MacroEngine::State s, const QString& macroId) {
    const bool started = m_state == MacroEngine::State::Idle && s == MacroEngine::State::Running;
    m_state = s;
    m_runningId = s == MacroEngine::State::Idle ? QString() : macroId;
    m_banner->setVisible(s == MacroEngine::State::WaitingConfirm);
    if (s == MacroEngine::State::WaitingConfirm) showAndRaise();
    if (s == MacroEngine::State::Idle) m_info->clear();
    if (started && !macroId.isEmpty()) pushRecent(macroId);
    updateStates();
    updateButtons();
}

void MainWindow::onProgress(const QString& macroId, int index, int loop) {
    const auto m = m_pm.macro(macroId);
    if (!m) return;
    const QString rep = m->infinite ? QStringLiteral("∞") : QString::number(m->repeatCount);
    m_info->setText(QStringLiteral("Выполняется «%1»: действие %2 из %3, проход %4 из %5")
                        .arg(m->name).arg(index + 1).arg(m->actions.size()).arg(loop).arg(rep));
}

// ------------------------------------------------------------------ горячие клавиши
QStringList MainWindow::applyHotkeys() {
    using C = HotkeyManager::Command;
    static const QList<QPair<QString, C>> map = {{"run", C::Run}, {"stop", C::Stop}, {"pause", C::Pause},
                                                  {"resume", C::Resume}, {"show", C::ShowApp}};
    QMap<C, Hotkey> global;
    for (const auto& p : map) {
        auto h = Hotkey::fromString(m_set.hotkey(p.first));
        if (!h) h = Hotkey::fromString(AppSettings::defaultHotkeys().value(p.first));
        if (h) global.insert(p.second, *h);
    }
    QMap<QString, Hotkey> perMacro;
    for (const Macro& m : m_pm.macros(m_pm.activeProfileId()))
        if (const auto h = Hotkey::fromString(m.hotkey)) perMacro.insert(m.id, *h);
    const QStringList errors = m_hk.apply(global, perMacro);
    if (m_hkWarn) {
        m_hkWarn->setVisible(!errors.isEmpty());
        m_hkWarn->setText("⚠ " + errors.join("\n⚠ "));
    }
    return errors;
}

// ------------------------------------------------------------------ страница «Профили»
QWidget* MainWindow::buildProfilesPage() {
    auto* page = new QWidget;
    auto* v = new QVBoxLayout(page);
    v->setContentsMargins(24, 20, 24, 18);
    v->setSpacing(14);
    auto* title = new QLabel("Профили");
    title->setObjectName("h1");
    v->addWidget(title);
    auto* sub = new QLabel("У каждого профиля свой набор макросов. Двойной клик — сделать профиль активным.");
    sub->setObjectName("muted");
    v->addWidget(sub);

    auto* tb = new QHBoxLayout;
    auto* bNew = makeButton("＋ Создать", "accent");
    auto* bRen = makeButton("Переименовать");
    auto* bAct = makeButton("Сделать активным");
    auto* bDel = makeButton("Удалить", "danger");
    for (auto* b : {bNew, bRen, bAct, bDel}) tb->addWidget(b);
    tb->addStretch();
    auto* bImp = makeButton("Импорт JSON");
    auto* bExp = makeButton("Экспорт JSON");
    tb->addWidget(bImp);
    tb->addWidget(bExp);
    v->addLayout(tb);

    m_profileList = new QListWidget;
    v->addWidget(m_profileList, 1);

    connect(bNew, &QPushButton::clicked, this, &MainWindow::createProfile);
    connect(bRen, &QPushButton::clicked, this, &MainWindow::renameProfile);
    connect(bAct, &QPushButton::clicked, this, &MainWindow::activateProfile);
    connect(bDel, &QPushButton::clicked, this, &MainWindow::deleteProfile);
    connect(bImp, &QPushButton::clicked, this, &MainWindow::importProfile);
    connect(bExp, &QPushButton::clicked, this, &MainWindow::exportProfile);
    connect(m_profileList, &QListWidget::itemDoubleClicked, this, [this] { activateProfile(); });
    return page;
}

void MainWindow::reloadProfileList() {
    if (!m_profileList) return;
    const QString keep = selectedProfileId();
    m_profileList->clear();
    const QString active = m_pm.activeProfileId();
    for (const Profile& p : m_pm.profiles()) {
        const int n = m_pm.macros(p.id).size();
        auto* it = new QListWidgetItem(QStringLiteral("%1%2   ·   макросов: %3").arg(p.name, p.id == active ? "   (активный)" : "").arg(n));
        it->setData(Qt::UserRole, p.id);
        if (p.id == active) it->setForeground(QColor("#ff6a00"));
        m_profileList->addItem(it);
        if (p.id == keep) m_profileList->setCurrentItem(it);
    }
    if (!m_profileList->currentItem() && m_profileList->count() > 0) m_profileList->setCurrentRow(0);
}

QString MainWindow::selectedProfileId() const {
    if (!m_profileList) return {};
    auto* it = m_profileList->currentItem();
    return it ? it->data(Qt::UserRole).toString() : QString();
}

void MainWindow::createProfile() {
    bool ok = false;
    const QString name = QInputDialog::getText(this, "Новый профиль", "Название профиля:", QLineEdit::Normal, {}, &ok);
    if (!ok) return;
    QString err;
    if (m_pm.createProfile(name, &err).isEmpty()) QMessageBox::warning(this, "Ошибка", err);
}

void MainWindow::renameProfile() {
    const QString id = selectedProfileId();
    if (id.isEmpty()) return;
    QString cur;
    for (const Profile& p : m_pm.profiles()) if (p.id == id) cur = p.name;
    bool ok = false;
    const QString name = QInputDialog::getText(this, "Переименовать", "Новое название:", QLineEdit::Normal, cur, &ok);
    if (!ok) return;
    QString err;
    if (!m_pm.renameProfile(id, name, &err)) QMessageBox::warning(this, "Ошибка", err);
}

void MainWindow::activateProfile() {
    const QString id = selectedProfileId();
    if (id.isEmpty() || m_state != MacroEngine::State::Idle) return;
    m_pm.setActiveProfile(id);
}

void MainWindow::deleteProfile() {
    const QString id = selectedProfileId();
    if (id.isEmpty()) return;
    if (QMessageBox::question(this, "Удаление", "Удалить профиль вместе со всеми его макросами?") != QMessageBox::Yes) return;
    QString err;
    if (!m_pm.deleteProfile(id, &err)) QMessageBox::warning(this, "Ошибка", err);
    else reloadMacros();
}

void MainWindow::importProfile() {
    const QString path = QFileDialog::getOpenFileName(this, "Импорт профиля", {}, "Профили HANDI (*.json)");
    if (path.isEmpty()) return;
    QString err;
    if (m_pm.importProfile(path, &err).isEmpty()) QMessageBox::warning(this, "Импорт не удался", err);
    else QMessageBox::information(this, "Импорт", "Профиль импортирован.");
}

void MainWindow::exportProfile() {
    const QString id = selectedProfileId();
    if (id.isEmpty()) return;
    QString name = "profile";
    for (const Profile& p : m_pm.profiles()) if (p.id == id) name = p.name;
    name.replace(QRegularExpression("[\\\\/:*?\"<>|]"), "_");
    const QString path = QFileDialog::getSaveFileName(this, "Экспорт профиля", name + ".json", "Профили HANDI (*.json)");
    if (path.isEmpty()) return;
    QString err;
    if (!m_pm.exportProfile(id, path, &err)) QMessageBox::warning(this, "Экспорт не удался", err);
    else QMessageBox::information(this, "Экспорт", "Профиль сохранён.");
}

// ------------------------------------------------------------------ страница «Настройки»
QWidget* MainWindow::buildSettingsPage() {
    auto* page = new QWidget;
    auto* v = new QVBoxLayout(page);
    v->setContentsMargins(24, 20, 24, 18);
    v->setSpacing(14);
    auto* title = new QLabel("Настройки");
    title->setObjectName("h1");
    v->addWidget(title);

    auto* keysPanel = new QFrame;
    keysPanel->setObjectName("panel");
    auto* kv = new QVBoxLayout(keysPanel);
    kv->setContentsMargins(18, 16, 18, 16);
    kv->setSpacing(10);
    auto* kt = new QLabel("Глобальные горячие клавиши");
    kt->setStyleSheet("font-weight:600;font-size:15px;");
    kv->addWidget(kt);
    const QList<QPair<QString, QString>> rows = {{"run", "Запуск выбранного макроса"}, {"stop", "Остановка"}, {"pause", "Пауза"},
                                                  {"resume", "Продолжение / подтверждение"}, {"show", "Открыть приложение"}};
    for (const auto& r : rows) {
        auto* row = new QHBoxLayout;
        auto* lab = new QLabel(r.second);
        lab->setMinimumWidth(240);
        auto* ed = new HotkeyEdit(HotkeyEdit::Mode::Combo);
        if (auto h = Hotkey::fromString(m_set.hotkey(r.first))) ed->setHotkey(*h);
        ed->setMaximumWidth(260);
        m_hkEdits.insert(r.first, ed);
        row->addWidget(lab);
        row->addWidget(ed);
        row->addStretch();
        kv->addLayout(row);
    }
    auto* kb = new QHBoxLayout;
    auto* save = makeButton("Сохранить", "accent");
    auto* reset = makeButton("Сбросить по умолчанию");
    kb->addWidget(save);
    kb->addWidget(reset);
    kb->addStretch();
    kv->addLayout(kb);
    m_hkResult = new QLabel;
    m_hkResult->setWordWrap(true);
    kv->addWidget(m_hkResult);
    v->addWidget(keysPanel);
    connect(save, &QPushButton::clicked, this, &MainWindow::saveHotkeySettings);
    connect(reset, &QPushButton::clicked, this, [this] {
        const auto defs = AppSettings::defaultHotkeys();
        for (auto it = m_hkEdits.cbegin(); it != m_hkEdits.cend(); ++it)
            if (auto h = Hotkey::fromString(defs.value(it.key()))) it.value()->setHotkey(*h);
        saveHotkeySettings();
    });

    auto* gen = new QFrame;
    gen->setObjectName("panel");
    auto* gv = new QVBoxLayout(gen);
    gv->setContentsMargins(18, 16, 18, 16);
    m_trayCheck = new QCheckBox("При закрытии окна сворачивать в трей (макросы и клавиши продолжают работать)");
    m_trayCheck->setChecked(m_set.getBool("tray.close", false));
    connect(m_trayCheck, &QCheckBox::toggled, this, [this](bool on) { m_set.setBool("tray.close", on); });
    gv->addWidget(m_trayCheck);
    auto* opRow = new QHBoxLayout;
    opRow->addWidget(new QLabel("Прозрачность окна"));
    auto* sl = new QSlider(Qt::Horizontal);
    sl->setRange(60, 100);
    sl->setValue(int(m_opacity * 100));
    sl->setMaximumWidth(260);
    opRow->addWidget(sl);
    opRow->addStretch();
    gv->addLayout(opRow);
    connect(sl, &QSlider::valueChanged, this, [this](int val) {
        m_opacity = val / 100.0;
        setWindowOpacity(m_opacity);
        m_set.set("opacity", QString::number(val));
    });
    v->addWidget(gen);

    auto* lic = new QFrame;
    lic->setObjectName("panel");
    auto* lv = new QVBoxLayout(lic);
    lv->setContentsMargins(18, 16, 18, 16);
    auto* lt = new QLabel("Лицензия");
    lt->setStyleSheet("font-weight:600;font-size:15px;");
    lv->addWidget(lt);
    m_licInfo = new QLabel("—");
    m_licInfo->setTextInteractionFlags(Qt::TextSelectableByMouse);
    lv->addWidget(m_licInfo);
    auto* lb = new QHBoxLayout;
    auto* check = makeButton("Проверить сейчас");
    auto* change = makeButton("Сменить ключ");
    lb->addWidget(check);
    lb->addWidget(change);
    lb->addStretch();
    lv->addLayout(lb);
    v->addWidget(lic);
    connect(check, &QPushButton::clicked, this, [this] { m_lic.verifyStored(); });
    connect(change, &QPushButton::clicked, this, [this] {
        if (QMessageBox::question(this, "Смена ключа", "Текущая лицензия будет удалена с этого компьютера. Продолжить?") == QMessageBox::Yes)
            emit licenseResetRequested();
    });

    auto* note = new QLabel("HANDI использует обычный ввод Windows и не взаимодействует с процессом игры. "
                            "Совместимость с защитой конкретной игры не проверялась — используйте на свой риск и в рамках правил игры.");
    note->setObjectName("muted");
    note->setWordWrap(true);
    v->addWidget(note);
    v->addStretch();
    return page;
}

void MainWindow::saveHotkeySettings() {
    QList<Hotkey> seen;
    for (auto it = m_hkEdits.cbegin(); it != m_hkEdits.cend(); ++it) {
        const Hotkey h = it.value()->hotkey();
        if (!h.isValid()) { m_hkResult->setObjectName("warn"); repolish(m_hkResult); m_hkResult->setText("Задайте сочетание для каждого действия."); return; }
        if (seen.contains(h)) { m_hkResult->setObjectName("warn"); repolish(m_hkResult); m_hkResult->setText("Одинаковые сочетания: " + h.toString()); return; }
        seen.append(h);
    }
    for (auto it = m_hkEdits.cbegin(); it != m_hkEdits.cend(); ++it) m_set.setHotkey(it.key(), it.value()->hotkey().toString());
    const QStringList errs = applyHotkeys();
    m_hkResult->setObjectName(errs.isEmpty() ? "muted" : "warn");
    repolish(m_hkResult);
    m_hkResult->setText(errs.isEmpty() ? "Сохранено." : errs.join("\n"));
}

void MainWindow::setLicenseStatus(const LicenseStatus& st) {
    QString t = st.isValid() ? "Статус: активна" : "Статус: " + st.message;
    if (st.isValid()) {
        t += st.lifetime ? "\nСрок: бессрочная" : "\nДействует до: " + st.expiresAt.toLocalTime().toString("dd.MM.yyyy HH:mm");
        if (st.offline) t += "\n(нет связи с сервером, используется сохранённая проверка)";
    }
    const QString key = m_lic.storedKeyMasked();
    if (!key.isEmpty()) t += "\nКлюч: " + key;
    m_licInfo->setText(t);
}

}
