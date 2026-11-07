#include "AppConfig.h"
#include "engine/MacroEngine.h"
#include "hotkeys/HotkeyManager.h"
#include "licensing/LicenseManager.h"
#include "profiles/ProfileManager.h"
#include "ui/ActivationDialog.h"
#include "ui/MainWindow.h"
#include "ui/Theme.h"
#include <QApplication>
#include <QDir>
#include <QIcon>
#include <QLockFile>
#include <QMessageBox>
#include <QStandardPaths>
#include <QTimer>

using namespace handi;

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    QApplication::setApplicationName(HANDI_APP_NAME);
    QApplication::setOrganizationName(HANDI_APP_NAME);
    QApplication::setApplicationVersion(HANDI_APP_VERSION);
    QApplication::setQuitOnLastWindowClosed(false);
    app.setStyle("Fusion");
    app.setStyleSheet(theme::styleSheet());
    app.setWindowIcon(QIcon(":/logo.png"));

    const QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dataDir);

    QLockFile lock(QDir::temp().filePath("handi.lock"));
    lock.setStaleLockTime(0);
    if (!lock.tryLock(100)) {
        QMessageBox::information(nullptr, "HANDI", "HANDI уже запущен. Откройте его из трея или клавишей F10.");
        return 0;
    }

    Database db(Database::defaultPath());
    QString err;
    if (!db.open(&err)) {
        QMessageBox::critical(nullptr, "HANDI", "Не удалось открыть базу данных:\n" + err);
        return 1;
    }
    AppSettings settings(db);
    ProfileManager profiles(db, settings);
    if (!profiles.init(&err)) {
        QMessageBox::critical(nullptr, "HANDI", "Ошибка инициализации профилей:\n" + err);
        return 1;
    }

    MacroEngine engine;
    HotkeyManager hotkeys;
    LicenseManager license(QUrl(HANDI_LICENSE_API_URL), dataDir + "/license.dat");
    ActivationDialog activation(license);

    MainWindow* win = nullptr;
    bool locked = true;

    auto lockApp = [&](const LicenseStatus& st) {
        locked = true;
        engine.stop();
        if (win) win->hide();
        if (!activation.isVisible()) activation.openFresh(st.message, true);
    };
    auto unlockApp = [&](const LicenseStatus& st) {
        if (!win) {
            win = new MainWindow(profiles, settings, engine, hotkeys, license);
            QObject::connect(win, &MainWindow::licenseResetRequested, [&] {
                license.removeLicense();
                lockApp(LicenseStatus{LicenseStatus::Result::None, "", "Введите новый лицензионный ключ"});
                activation.openFresh("Введите новый лицензионный ключ", false);
            });
        }
        win->setLicenseStatus(st);
        if (locked) {
            locked = false;
            activation.hide();
            win->showAndRaise();
        }
    };

    QObject::connect(&license, &LicenseManager::finished, &app, [&](const LicenseStatus& st) {
        if (st.isValid()) unlockApp(st);
        else if (st.result == LicenseStatus::Result::None && !win) activation.openFresh();
        else lockApp(st);
    });

    // Периодическая перепроверка лицензии (блокировка, истечение срока).
    QTimer recheck;
    recheck.setInterval(30 * 60 * 1000);
    QObject::connect(&recheck, &QTimer::timeout, &license, [&] { if (!locked) license.verifyStored(); });
    recheck.start();

    if (license.hasStoredLicense()) license.verifyStored();
    else activation.openFresh();

    const int rc = app.exec();
    engine.stop();
    engine.wait(2000);
    delete win;
    return rc;
}
