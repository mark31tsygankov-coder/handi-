#pragma once
#include <QJsonObject>
#include <QList>
#include <QString>
#include <QUuid>

namespace handi {

enum class ActionType { KeyDown, KeyUp, KeyPress, MouseClick, Delay, WaitConfirm };
enum class MouseButton { Left, Right, Middle };

struct Action {
    ActionType type = ActionType::KeyPress;
    quint32 vk = 0;                          // для клавиатурных действий
    MouseButton button = MouseButton::Left;  // для MouseClick
    bool useCoords = false;                  // кликнуть в точке экрана
    int x = 0, y = 0;
    int delayMs = 50;                        // пауза ПОСЛЕ действия

    QJsonObject toJson() const;
    static Action fromJson(const QJsonObject& o);
    QString describe() const;                // текст для визуальной последовательности
};

struct Macro {
    QString id;
    QString profileId;
    QString name;
    QList<Action> actions;
    int repeatCount = 1;
    bool infinite = false;
    QString hotkey;                          // запуск/остановка, например "Ctrl+F5"

    QJsonObject toJson() const;
    static Macro fromJson(const QJsonObject& o);
    static QString newId() { return QUuid::createUuid().toString(QUuid::WithoutBraces); }
};

constexpr int kMaxDelayMs = 3600000;
constexpr int kMaxRepeat = 1000000;
constexpr int kMaxActions = 10000;

} // namespace handi
