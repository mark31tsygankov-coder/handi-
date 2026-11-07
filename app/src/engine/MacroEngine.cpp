#include "engine/MacroEngine.h"
#include "engine/InputSender.h"
#include <QDeadlineTimer>
#include <QSet>

namespace handi {

MacroEngine::MacroEngine(QObject* parent) : QThread(parent) {}

MacroEngine::~MacroEngine() {
    stop();
    wait(3000);
}

MacroEngine::State MacroEngine::state() const { QMutexLocker l(&m_mutex); return m_state; }
QString MacroEngine::currentMacroId() const { QMutexLocker l(&m_mutex); return m_macro.id; }

void MacroEngine::setState(State s) {
    QString id;
    {
        QMutexLocker l(&m_mutex);
        if (m_state == s) return;
        m_state = s;
        id = m_macro.id;
    }
    emit stateChanged(s, id);
}

bool MacroEngine::startMacro(const Macro& m) {
    if (m.actions.isEmpty()) return false;
    if (isRunning()) {
        if (state() == State::Idle) wait(500);   // поток как раз завершается
        if (isRunning()) return false;
    }
    {
        QMutexLocker l(&m_mutex);
        m_macro = m;
        m_stop = false;
        m_pause = false;
    }
    start();
    return true;
}

void MacroEngine::stop() {
    QMutexLocker l(&m_mutex);
    m_stop = true;
    m_pause = false;
    m_cond.wakeAll();
}

void MacroEngine::pause() {
    {
        QMutexLocker l(&m_mutex);
        if (m_state != State::Running) return;
        m_pause = true;
    }
    setState(State::Paused);
}

void MacroEngine::resume() {
    {
        QMutexLocker l(&m_mutex);
        if (!m_pause) return;
        m_pause = false;
        m_cond.wakeAll();
    }
    setState(State::Running);
}

bool MacroEngine::checkpoint() {
    QMutexLocker l(&m_mutex);
    while (m_pause && !m_stop) m_cond.wait(&m_mutex);
    return !m_stop;
}

bool MacroEngine::sleepInterruptible(int ms) {
    QMutexLocker l(&m_mutex);
    QDeadlineTimer dl(ms);
    while (!m_stop && !dl.hasExpired()) m_cond.wait(&m_mutex, dl);
    return !m_stop;
}

void MacroEngine::run() {
    Macro m;
    { QMutexLocker l(&m_mutex); m = m_macro; }
    setState(State::Running);

    QSet<quint32> held;
    bool ok = true;
    for (int loop = 1; ok && (m.infinite || loop <= m.repeatCount); ++loop) {
        for (int i = 0; i < m.actions.size() && ok; ++i) {
            if (!checkpoint()) { ok = false; break; }
            const Action& a = m.actions[i];
            emit progress(m.id, i, loop);
            switch (a.type) {
            case ActionType::KeyDown:
                input::key(a.vk, true);
                held.insert(a.vk);
                break;
            case ActionType::KeyUp:
                input::key(a.vk, false);
                held.remove(a.vk);
                break;
            case ActionType::KeyPress:
                input::key(a.vk, true);
                ok = sleepInterruptible(25);
                input::key(a.vk, false);
                break;
            case ActionType::MouseClick:
                if (a.useCoords) input::moveTo(a.x, a.y);
                input::mouseButton(a.button, true);
                ok = sleepInterruptible(25);
                input::mouseButton(a.button, false);
                break;
            case ActionType::Delay:
                break;
            case ActionType::WaitConfirm:
                { QMutexLocker l(&m_mutex); m_pause = true; }
                setState(State::WaitingConfirm);
                if (!checkpoint()) ok = false; else setState(State::Running);
                break;
            }
            if (ok) ok = sleepInterruptible(a.delayMs);
        }
    }
    for (quint32 vk : std::as_const(held)) input::key(vk, false);  // не оставляем зажатые клавиши
    setState(State::Idle);
}

} // namespace handi
