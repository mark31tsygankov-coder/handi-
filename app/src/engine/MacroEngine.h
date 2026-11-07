#pragma once
#include "macro/Macro.h"
#include <QMutex>
#include <QThread>
#include <QWaitCondition>

namespace handi {

// Выполняет макрос в отдельном потоке. Остановка, пауза и продолжение работают в любой момент.
// Действие WaitConfirm ставит макрос на паузу до ручного подтверждения (кнопка/«Продолжить»):
// так макрос никогда не пытается сам проходить проверки игры вроде «Вы человек?».
class MacroEngine : public QThread {
    Q_OBJECT
public:
    enum class State { Idle, Running, Paused, WaitingConfirm };
    Q_ENUM(State)

    explicit MacroEngine(QObject* parent = nullptr);
    ~MacroEngine() override;

    bool startMacro(const Macro& m);  // false, если уже выполняется другой макрос или макрос пуст
    void stop();
    void pause();
    void resume();                    // также подтверждает WaitConfirm

    State state() const;
    QString currentMacroId() const;

signals:
    void stateChanged(handi::MacroEngine::State state, const QString& macroId);
    void progress(const QString& macroId, int actionIndex, int loop);  // loop считается с 1

protected:
    void run() override;

private:
    void setState(State s);
    bool checkpoint();               // блокируется на паузе; false — запрошена остановка
    bool sleepInterruptible(int ms); // false — запрошена остановка

    mutable QMutex m_mutex;
    QWaitCondition m_cond;
    Macro m_macro;
    State m_state = State::Idle;
    bool m_stop = false;
    bool m_pause = false;
};

} // namespace handi
