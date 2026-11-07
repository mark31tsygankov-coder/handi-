#pragma once
#include "macro/Macro.h"
#include <QPoint>

// Обычный пользовательский ввод Windows через SendInput.
// Никакого доступа к процессу или памяти игры.
namespace handi::input {
void key(quint32 vk, bool down);
void mouseButton(MouseButton b, bool down);
QPoint cursorPos();          // текущая позиция курсора в физических пикселях
void moveTo(int x, int y);   // координаты виртуального рабочего стола, в пикселях
}
