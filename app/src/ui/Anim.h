#pragma once
#include <QEasingCurve>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QWidget>

namespace handi::anim {

// Плавное появление виджета.
inline void fadeIn(QWidget* w, int ms = 240) {
    if (!w) return;
    auto* eff = new QGraphicsOpacityEffect(w);
    w->setGraphicsEffect(eff);
    auto* a = new QPropertyAnimation(eff, "opacity", w);
    a->setDuration(ms);
    a->setStartValue(0.0);
    a->setEndValue(1.0);
    a->setEasingCurve(QEasingCurve::OutCubic);
    QObject::connect(a, &QPropertyAnimation::finished, w, [w] { w->setGraphicsEffect(nullptr); });
    a->start(QAbstractAnimation::DeleteWhenStopped);
}

}
