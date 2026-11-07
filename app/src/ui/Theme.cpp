#include "ui/Theme.h"

namespace handi::theme {

QString styleSheet() {
    return QStringLiteral(R"(
* { font-family: "Segoe UI"; font-size: 14px; color: #f4f4f5; }
QWidget { background: transparent; }
QMainWindow { background: transparent; }
QDialog, QMessageBox, QInputDialog, QFileDialog { background: #0e0e10; }
QToolTip { background: #19191d; color: #f4f4f5; border: 1px solid #ff6a00; padding: 4px 8px; border-radius: 6px; }

QFrame#window { background: #0e0e10; border: 1px solid #2a2a30; border-radius: 22px; }
QFrame#topbar { background: #0e0e10; border: none; border-bottom: 1px solid #1d1d21; border-top-left-radius: 22px; border-top-right-radius: 22px; }

QLabel#h1 { font-size: 24px; font-weight: 700; }
QLabel#brand { font-size: 17px; font-weight: 800; color: #ff7a1a; letter-spacing: 3px; }
QLabel#section { font-size: 15px; font-weight: 700; }
QLabel#muted { color: #9a9aa2; }
QLabel#warn { color: #ff9a4d; }
QLabel#chip { background: #1f1f24; border-radius: 10px; padding: 3px 10px; color: #d0d0d6; }
QLabel#state { font-weight: 600; color: #9a9aa2; }
QLabel#cardTitle { font-size: 18px; font-weight: 700; }
QLabel#tileTitle { font-size: 17px; font-weight: 700; }
QLabel#tileIcon { background: #2a1a10; color: #ff7a1a; border-radius: 12px; font-size: 18px; }
QLabel#tileIconAccent { background: qlineargradient(x1:0,y1:0,x2:1,y2:1, stop:0 #ff8a2a, stop:1 #ff5a00); color: #000; border-radius: 12px; font-size: 20px; font-weight: 700; }
QLabel#pill { background: #151518; border: 1px solid #26262b; border-radius: 16px; padding: 6px 14px; font-weight: 600; }

QFrame#panel { background: #131316; border: 1px solid #232328; border-radius: 16px; }
QFrame#banner { background: #2a170a; border: 1px solid #ff6a00; border-radius: 14px; }
QFrame#tile { background: #151518; border: 1px solid #232328; border-radius: 18px; }
QFrame#tile:hover { background: #1a1a1e; border-color: #3a3a42; }
QFrame#tile[accentTile="true"] { background: #1c140e; border-color: #4a2a12; }
QFrame#tile[accentTile="true"]:hover { border-color: #ff6a00; }
QFrame#macroCard { background: #151518; border: 1px solid #232328; border-radius: 18px; }
QFrame#macroCard:hover { border-color: #3a3a42; }
QFrame#macroCard[selected="true"] { background: #1b1511; border: 1px solid #ff6a00; }
QFrame#recent { background: #151518; border: 1px solid #232328; border-radius: 14px; }
QFrame#recent:hover { border-color: #ff6a00; }

QPushButton { background: #222227; border: 1px solid #2c2c33; border-radius: 14px; padding: 10px 18px; }
QPushButton:hover { background: #2c2c33; border-color: #3a3a42; }
QPushButton:pressed { background: #1b1b20; }
QPushButton:disabled { background: #161618; color: #55555d; border-color: #1f1f22; }
QPushButton[accent="true"] { background: qlineargradient(x1:0,y1:0,x2:1,y2:1, stop:0 #ff8a2a, stop:1 #ff5a00); color: #000; font-weight: 700; border: none; }
QPushButton[accent="true"]:hover { background: qlineargradient(x1:0,y1:0,x2:1,y2:1, stop:0 #ff9a45, stop:1 #ff6a1a); }
QPushButton[accent="true"]:disabled { background: #3a2412; color: #7a6a5a; }
QPushButton[danger="true"] { background: #3a1618; color: #ffb4b6; border-color: #5a1d20; }
QPushButton[danger="true"]:hover { background: #5a1d20; }

QPushButton#navpill { background: transparent; border: none; border-radius: 17px; padding: 8px 18px; color: #b5b5bc; font-weight: 500; }
QPushButton#navpill:hover { background: #1a1a1e; color: #fff; }
QPushButton#navpill:checked { background: #2a170a; color: #ff7a1a; font-weight: 700; }

QPushButton#ico { background: #1a1a1e; border: 1px solid #26262b; border-radius: 12px; padding: 0; min-width: 36px; max-width: 36px; min-height: 36px; max-height: 36px; font-size: 15px; }
QPushButton#ico:hover { background: #26262c; border-color: #3a3a42; }
QPushButton#ico:checked { background: #2a170a; border-color: #ff6a00; color: #ff7a1a; }
QPushButton#ico[accent="true"] { background: qlineargradient(x1:0,y1:0,x2:1,y2:1, stop:0 #ff8a2a, stop:1 #ff5a00); border: none; color: #000; }
QPushButton#ico[danger="true"]:hover { background: #5a1d20; border-color: #7a2428; }
QPushButton#keychip { background: #1f1f24; border: 1px solid #2c2c33; border-radius: 15px; padding: 6px 14px; color: #e4e4ea; font-weight: 600; }
QPushButton#keychip:hover { border-color: #ff6a00; color: #ff7a1a; }

QLineEdit, QSpinBox, QComboBox { background: #111113; border: 1px solid #26262b; border-radius: 12px; padding: 9px 12px; selection-background-color: #ff6a00; selection-color: #000; }
QLineEdit:focus, QSpinBox:focus, QComboBox:focus { border: 1px solid #ff6a00; }
QLineEdit#search { background: #151518; border-radius: 19px; padding: 9px 18px; font-size: 15px; }
QComboBox::drop-down { border: none; width: 26px; }
QComboBox QAbstractItemView { background: #19191d; border: 1px solid #2a2a30; selection-background-color: #3a2210; outline: none; }
QSpinBox::up-button, QSpinBox::down-button { width: 18px; border: none; }

QListWidget { background: #131316; border: 1px solid #232328; border-radius: 14px; padding: 6px; outline: none; }
QListWidget::item { padding: 8px 10px; border-radius: 8px; }
QListWidget::item:hover { background: #1c1c1f; }
QListWidget::item:selected { background: #3a2210; color: #ffffff; }

QCheckBox { spacing: 8px; }
QCheckBox::indicator { width: 18px; height: 18px; border-radius: 6px; border: 1px solid #3a3a40; background: #111113; }
QCheckBox::indicator:checked { background: #ff6a00; border-color: #ff6a00; }
QSlider::groove:horizontal { height: 6px; background: #26262b; border-radius: 3px; }
QSlider::sub-page:horizontal { background: #ff6a00; border-radius: 3px; }
QSlider::handle:horizontal { background: #ffffff; width: 16px; margin: -6px 0; border-radius: 8px; }

QScrollArea { background: transparent; border: none; }
QScrollBar:vertical { background: transparent; width: 10px; margin: 2px; }
QScrollBar::handle:vertical { background: #34343a; border-radius: 4px; min-height: 30px; }
QScrollBar::handle:vertical:hover { background: #ff6a00; }
QScrollBar::add-line, QScrollBar::sub-line { height: 0; width: 0; }
QScrollBar:horizontal { height: 0; }
QMenu { background: #19191d; border: 1px solid #2a2a30; padding: 6px; border-radius: 10px; }
QMenu::item { padding: 8px 22px; border-radius: 6px; }
QMenu::item:selected { background: #3a2210; }
)");
}

} // namespace handi::theme
