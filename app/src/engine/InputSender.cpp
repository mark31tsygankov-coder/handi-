#include "engine/InputSender.h"
#include <windows.h>

namespace handi::input {

static bool isExtended(UINT vk) {
    switch (vk) {
    case VK_INSERT: case VK_DELETE: case VK_HOME: case VK_END: case VK_PRIOR: case VK_NEXT:
    case VK_LEFT: case VK_UP: case VK_RIGHT: case VK_DOWN: case VK_DIVIDE: case VK_NUMLOCK:
        return true;
    default: return false;
    }
}

void key(quint32 vk, bool down) {
    INPUT in{};
    in.type = INPUT_KEYBOARD;
    const UINT sc = MapVirtualKeyW(vk, MAPVK_VK_TO_VSC);
    if (sc) { in.ki.wScan = WORD(sc); in.ki.dwFlags = KEYEVENTF_SCANCODE; }
    else in.ki.wVk = WORD(vk);
    if (isExtended(vk)) in.ki.dwFlags |= KEYEVENTF_EXTENDEDKEY;
    if (!down) in.ki.dwFlags |= KEYEVENTF_KEYUP;
    SendInput(1, &in, sizeof(INPUT));
}

void mouseButton(MouseButton b, bool down) {
    INPUT in{};
    in.type = INPUT_MOUSE;
    switch (b) {
    case MouseButton::Left: in.mi.dwFlags = down ? MOUSEEVENTF_LEFTDOWN : MOUSEEVENTF_LEFTUP; break;
    case MouseButton::Right: in.mi.dwFlags = down ? MOUSEEVENTF_RIGHTDOWN : MOUSEEVENTF_RIGHTUP; break;
    case MouseButton::Middle: in.mi.dwFlags = down ? MOUSEEVENTF_MIDDLEDOWN : MOUSEEVENTF_MIDDLEUP; break;
    }
    SendInput(1, &in, sizeof(INPUT));
}

QPoint cursorPos() {
    POINT p{};
    GetCursorPos(&p);
    return QPoint(p.x, p.y);
}

void moveTo(int x, int y) {
    const int vx = GetSystemMetrics(SM_XVIRTUALSCREEN), vy = GetSystemMetrics(SM_YVIRTUALSCREEN);
    const int vw = GetSystemMetrics(SM_CXVIRTUALSCREEN), vh = GetSystemMetrics(SM_CYVIRTUALSCREEN);
    if (vw <= 1 || vh <= 1) return;
    INPUT in{};
    in.type = INPUT_MOUSE;
    in.mi.dx = LONG((qint64(x - vx) * 65535) / (vw - 1));
    in.mi.dy = LONG((qint64(y - vy) * 65535) / (vh - 1));
    in.mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_VIRTUALDESK;
    SendInput(1, &in, sizeof(INPUT));
}

} // namespace handi::input
