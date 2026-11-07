// Самопроверка ядра HANDI: модели, горячие клавиши, SQLite, профили, импорт/экспорт, движок.
// Ввод в систему НЕ отправляется: используются только действия «Пауза» и «Ждать подтверждения».
#include "engine/MacroEngine.h"
#include "hotkeys/Hotkey.h"
#include "macro/KeyNames.h"
#include "profiles/ProfileManager.h"
#include <QCoreApplication>
#include <QEventLoop>
#include <QTemporaryDir>
#include <QTimer>
#include <cstdio>

using namespace handi;

static int g_fail = 0;
#define CHECK(cond) do { if (cond) std::printf("  ok   %s\n", #cond); else { std::printf("  FAIL %s (line %d)\n", #cond, __LINE__); ++g_fail; } } while (0)

static Macro delayMacro(int actions, int delay, int repeat, bool infinite = false) {
    Macro m; m.id = "t"; m.name = "t"; m.repeatCount = repeat; m.infinite = infinite;
    for (int i = 0; i < actions; ++i) { Action a; a.type = ActionType::Delay; a.delayMs = delay; m.actions.append(a); }
    return m;
}

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);

    std::printf("[keys/hotkeys]\n");
    CHECK(keys::name(0x70) == "F1");
    CHECK(keys::fromName("f6") == 0x75);
    auto hk = Hotkey::fromString("Ctrl+Shift+F5");
    CHECK(hk.has_value() && hk->mods == (Hotkey::Ctrl | Hotkey::Shift) && hk->vk == 0x74);
    CHECK(hk->toString() == "Ctrl+Shift+F5");
    CHECK(!Hotkey::fromString("Ctrl+Shift").has_value());
    CHECK(!Hotkey::fromString("Foo+A").has_value());

    std::printf("[macro json]\n");
    Macro m; m.name = "Тест"; m.repeatCount = 3; m.hotkey = "Alt+F2";
    Action a; a.type = ActionType::KeyPress; a.vk = 'A'; a.delayMs = 120; m.actions.append(a);
    Action c; c.type = ActionType::MouseClick; c.useCoords = true; c.x = 100; c.y = 200; m.actions.append(c);
    const Macro m2 = Macro::fromJson(m.toJson());
    CHECK(m2.actions.size() == 2 && m2.actions[0].vk == 'A' && m2.actions[1].useCoords && m2.actions[1].y == 200);
    CHECK(m2.repeatCount == 3 && m2.hotkey == "Alt+F2");

    std::printf("[database/profiles]\n");
    QTemporaryDir dir;
    Database db(dir.filePath("t.db"));
    QString err;
    CHECK(db.open(&err));
    AppSettings settings(db);
    ProfileManager pm(db, settings);
    CHECK(pm.init(&err));
    CHECK(pm.profiles().size() == 1);
    const QString p2 = pm.createProfile("Второй", &err);
    CHECK(!p2.isEmpty() && pm.profiles().size() == 2);
    Macro sm = m; sm.profileId = p2;
    CHECK(pm.saveMacro(sm, &err));
    CHECK(pm.macros(p2).size() == 1 && pm.macros(pm.activeProfileId()).isEmpty());
    sm.name = "Переименован";
    CHECK(pm.saveMacro(sm) && pm.macro(sm.id)->name == "Переименован");
    const QString dup = pm.duplicateMacro(sm.id);
    CHECK(!dup.isEmpty() && pm.macros(p2).size() == 2);
    CHECK(pm.exportProfile(p2, dir.filePath("p.json"), &err));
    const QString imp = pm.importProfile(dir.filePath("p.json"), &err);
    CHECK(!imp.isEmpty() && pm.macros(imp).size() == 2);
    CHECK(pm.deleteProfile(p2) && pm.macros(p2).isEmpty());
    CHECK(settings.hotkey("run") == "F6");
    settings.setHotkey("run", "F5");
    CHECK(settings.hotkey("run") == "F5");

    std::printf("[engine]\n");
    {
        MacroEngine eng;
        int progress = 0;
        QObject::connect(&eng, &MacroEngine::progress, &app, [&](const QString&, int, int) { ++progress; });
        QEventLoop loop;
        QObject::connect(&eng, &MacroEngine::stateChanged, &loop, [&](MacroEngine::State s, const QString&) {
            if (s == MacroEngine::State::Idle) loop.quit();
        });
        QTimer::singleShot(5000, &loop, &QEventLoop::quit);
        CHECK(eng.startMacro(delayMacro(3, 20, 2)));
        loop.exec();
        eng.wait(1000);
        CHECK(eng.state() == MacroEngine::State::Idle);
        QCoreApplication::processEvents();
        CHECK(progress == 6);

        CHECK(eng.startMacro(delayMacro(2, 50, 1, true)));
        QThread::msleep(150);
        eng.pause();
        CHECK(eng.state() == MacroEngine::State::Paused);
        eng.resume();
        CHECK(eng.state() == MacroEngine::State::Running);
        eng.stop();
        eng.wait(2000);
        CHECK(eng.state() == MacroEngine::State::Idle);

        Macro w = delayMacro(1, 10, 1);
        Action wa; wa.type = ActionType::WaitConfirm; wa.delayMs = 0;
        w.actions.prepend(wa);
        CHECK(eng.startMacro(w));
        QThread::msleep(200);
        CHECK(eng.state() == MacroEngine::State::WaitingConfirm);
        eng.resume();
        eng.wait(2000);
        CHECK(eng.state() == MacroEngine::State::Idle);
    }

    std::printf(g_fail ? "\nПРОВАЛЕНО: %d\n" : "\nВсе проверки пройдены\n", g_fail);
    return g_fail ? 1 : 0;
}
