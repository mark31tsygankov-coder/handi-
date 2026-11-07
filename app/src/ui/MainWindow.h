#pragma once
#include "engine/MacroEngine.h"
#include "hotkeys/HotkeyManager.h"
#include "licensing/LicenseManager.h"
#include "profiles/ProfileManager.h"
#include <QHash>
#include <QMainWindow>
#include <QMap>
#include <QTimer>

class QCheckBox;
class QComboBox;
class QFrame;
class QHBoxLayout;
class QLabel;
class QLineEdit;
class QListWidget;
class QPushButton;
class QScrollArea;
class QStackedWidget;
class QSystemTrayIcon;
class QVBoxLayout;

namespace handi {
class HotkeyEdit;

// Оверлей: окно без рамки, поверх игры, с плитками и карточками макросов.
class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(ProfileManager& pm, AppSettings& settings, MacroEngine& engine, HotkeyManager& hotkeys,
               LicenseManager& license, QWidget* parent = nullptr);

    void showAndRaise();
    void setLicenseStatus(const LicenseStatus& st);

signals:
    void licenseResetRequested();

protected:
    void closeEvent(QCloseEvent* e) override;
    bool eventFilter(QObject* obj, QEvent* e) override;

private:
    QWidget* buildMacrosPage();
    QWidget* buildProfilesPage();
    QWidget* buildSettingsPage();
    QFrame* buildCard(const Macro& m);
    void setPage(int index);
    void toggleVisible();

    void reloadProfiles();
    void reloadMacros();
    void reloadRecent();
    void reloadProfileList();
    void pushRecent(const QString& id);
    QStringList applyHotkeys();

    void selectMacro(const QString& id);
    void createMacro();
    void editMacroById(const QString& id);
    void deleteMacroById(const QString& id);
    void duplicateMacroById(const QString& id);
    void assignHotkey(const QString& id);
    void runMacroById(const QString& id, bool immediate);
    void runSelected(bool immediate);
    void startMacro(const Macro& m);
    void cancelPending();
    void onCommand(HotkeyManager::Command cmd);
    void onMacroHotkey(const QString& id);
    void onEngineState(MacroEngine::State s, const QString& macroId);
    void onProgress(const QString& macroId, int index, int loop);
    void updateStates();
    void updateButtons();
    void saveHotkeySettings();

    void createProfile();
    void renameProfile();
    void deleteProfile();
    void activateProfile();
    void importProfile();
    void exportProfile();
    QString selectedProfileId() const;

    ProfileManager& m_pm;
    AppSettings& m_set;
    MacroEngine& m_engine;
    HotkeyManager& m_hk;
    LicenseManager& m_lic;

    QFrame* m_topbar = nullptr;
    QLineEdit* m_search = nullptr;
    QPushButton* m_pin = nullptr;
    QStackedWidget* m_stack = nullptr;
    QList<QPushButton*> m_nav;
    QComboBox* m_profileCombo = nullptr;

    QScrollArea* m_scroll = nullptr;
    QWidget* m_cardsHost = nullptr;
    QVBoxLayout* m_hostLayout = nullptr;
    QWidget* m_gridHost = nullptr;
    QHash<QString, QLabel*> m_stateLabels;
    QHash<QString, QFrame*> m_cards;
    QString m_selectedId;

    QLabel* m_recentTitle = nullptr;
    QHBoxLayout* m_recentLayout = nullptr;

    QPushButton *m_btnRun = nullptr, *m_btnStop = nullptr, *m_btnPause = nullptr, *m_btnResume = nullptr;
    QFrame* m_banner = nullptr;
    QLabel* m_pill = nullptr;
    QLabel* m_info = nullptr;
    QLabel* m_hkWarn = nullptr;

    QListWidget* m_profileList = nullptr;

    QMap<QString, HotkeyEdit*> m_hkEdits;
    QLabel* m_hkResult = nullptr;
    QCheckBox* m_trayCheck = nullptr;
    QLabel* m_licInfo = nullptr;
    QSystemTrayIcon* m_tray = nullptr;

    MacroEngine::State m_state = MacroEngine::State::Idle;
    QString m_runningId;
    QTimer m_startTimer;
    int m_countdown = 0;
    Macro m_pending;
    bool m_loading = false;
    double m_opacity = 1.0;
};

}
