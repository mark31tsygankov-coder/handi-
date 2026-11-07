#include "macro/Macro.h"
#include "macro/KeyNames.h"
#include <QJsonArray>
#include <algorithm>

namespace handi {

static QString typeToStr(ActionType t) {
    switch (t) {
    case ActionType::KeyDown: return "key_down";
    case ActionType::KeyUp: return "key_up";
    case ActionType::KeyPress: return "key_press";
    case ActionType::MouseClick: return "mouse_click";
    case ActionType::Delay: return "delay";
    case ActionType::WaitConfirm: return "wait_confirm";
    }
    return "delay";
}
static ActionType strToType(const QString& s) {
    if (s == "key_down") return ActionType::KeyDown;
    if (s == "key_up") return ActionType::KeyUp;
    if (s == "key_press") return ActionType::KeyPress;
    if (s == "mouse_click") return ActionType::MouseClick;
    if (s == "wait_confirm") return ActionType::WaitConfirm;
    return ActionType::Delay;
}
static QString btnToStr(MouseButton b) {
    return b == MouseButton::Right ? "right" : b == MouseButton::Middle ? "middle" : "left";
}
static MouseButton strToBtn(const QString& s) {
    return s == "right" ? MouseButton::Right : s == "middle" ? MouseButton::Middle : MouseButton::Left;
}

QJsonObject Action::toJson() const {
    QJsonObject o{{"type", typeToStr(type)}, {"delay_ms", delayMs}};
    if (type == ActionType::KeyDown || type == ActionType::KeyUp || type == ActionType::KeyPress)
        o["vk"] = int(vk);
    if (type == ActionType::MouseClick) {
        o["button"] = btnToStr(button);
        if (useCoords) { o["x"] = x; o["y"] = y; }
    }
    return o;
}

Action Action::fromJson(const QJsonObject& o) {
    Action a;
    a.type = strToType(o.value("type").toString());
    a.vk = quint32(std::clamp(o.value("vk").toInt(0), 0, 255));
    a.button = strToBtn(o.value("button").toString());
    a.useCoords = o.contains("x") && o.contains("y");
    a.x = std::clamp(o.value("x").toInt(0), -32768, 32768);
    a.y = std::clamp(o.value("y").toInt(0), -32768, 32768);
    a.delayMs = std::clamp(o.value("delay_ms").toInt(50), 0, kMaxDelayMs);
    return a;
}

QString Action::describe() const {
    switch (type) {
    case ActionType::KeyDown: return QStringLiteral("Нажать и держать: %1").arg(keys::name(vk));
    case ActionType::KeyUp: return QStringLiteral("Отпустить: %1").arg(keys::name(vk));
    case ActionType::KeyPress: return QStringLiteral("Нажать: %1").arg(keys::name(vk));
    case ActionType::MouseClick: {
        QString b = button == MouseButton::Right ? "ПКМ" : button == MouseButton::Middle ? "СКМ" : "ЛКМ";
        return useCoords ? QStringLiteral("Клик %1 (%2, %3)").arg(b).arg(x).arg(y)
                         : QStringLiteral("Клик %1 (текущая позиция)").arg(b);
    }
    case ActionType::Delay: return QStringLiteral("Пауза");
    case ActionType::WaitConfirm: return QStringLiteral("Ждать подтверждения пользователя");
    }
    return {};
}

QJsonObject Macro::toJson() const {
    QJsonArray arr;
    for (const Action& a : actions) arr.append(a.toJson());
    return QJsonObject{{"id", id}, {"name", name}, {"repeat_count", repeatCount},
                       {"infinite", infinite}, {"hotkey", hotkey}, {"actions", arr}};
}

Macro Macro::fromJson(const QJsonObject& o) {
    Macro m;
    m.id = o.value("id").toString();
    m.name = o.value("name").toString().trimmed().left(80);
    m.repeatCount = std::clamp(o.value("repeat_count").toInt(1), 1, kMaxRepeat);
    m.infinite = o.value("infinite").toBool(false);
    m.hotkey = o.value("hotkey").toString().left(40);
    const QJsonArray arr = o.value("actions").toArray();
    for (const QJsonValue& v : arr) {
        if (m.actions.size() >= kMaxActions) break;
        if (v.isObject()) m.actions.append(Action::fromJson(v.toObject()));
    }
    return m;
}

} // namespace handi
