#include "ui/ActivationDialog.h"
#include "ui/Anim.h"
#include <QApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPixmap>
#include <QPushButton>
#include <QRegularExpression>
#include <QStyle>
#include <QVBoxLayout>

namespace handi {

ActivationDialog::ActivationDialog(LicenseManager& lic, QWidget* parent) : QDialog(parent), m_lic(lic) {
    setWindowTitle("HANDI — активация");
    setWindowFlag(Qt::WindowContextHelpButtonHint, false);
    setFixedSize(460, 560);

    auto* v = new QVBoxLayout(this);
    v->setContentsMargins(36, 30, 36, 28);
    v->setSpacing(12);

    auto* logo = new QLabel;
    logo->setPixmap(QPixmap(":/logo.png").scaled(150, 150, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    logo->setAlignment(Qt::AlignCenter);
    v->addWidget(logo);

    auto* brand = new QLabel("HANDI");
    brand->setObjectName("brand");
    brand->setAlignment(Qt::AlignCenter);
    v->addWidget(brand);

    auto* sub = new QLabel("Введите лицензионный ключ для активации программы");
    sub->setObjectName("muted");
    sub->setAlignment(Qt::AlignCenter);
    sub->setWordWrap(true);
    v->addWidget(sub);
    v->addSpacing(8);

    m_key = new QLineEdit;
    m_key->setPlaceholderText("HANDI-XXXX-XXXX-XXXX-XXXX");
    m_key->setMaxLength(32);
    m_key->setAlignment(Qt::AlignCenter);
    v->addWidget(m_key);
    connect(m_key, &QLineEdit::textEdited, this, [this](const QString& t) {
        const int pos = m_key->cursorPosition();
        m_key->setText(t.toUpper());
        m_key->setCursorPosition(pos);
    });
    connect(m_key, &QLineEdit::returnPressed, this, &ActivationDialog::onActivate);

    m_btn = new QPushButton("Активировать");
    m_btn->setProperty("accent", true);
    m_btn->setMinimumHeight(42);
    v->addWidget(m_btn);
    connect(m_btn, &QPushButton::clicked, this, &ActivationDialog::onActivate);

    m_msg = new QLabel;
    m_msg->setWordWrap(true);
    m_msg->setAlignment(Qt::AlignCenter);
    m_msg->setMinimumHeight(48);
    v->addWidget(m_msg);

    v->addStretch();
    auto* dev = new QLabel("ID устройства: " + m_lic.deviceId().left(12));
    dev->setObjectName("muted");
    dev->setAlignment(Qt::AlignCenter);
    dev->setTextInteractionFlags(Qt::TextSelectableByMouse);
    v->addWidget(dev);

    connect(&m_lic, &LicenseManager::finished, this, [this](const LicenseStatus& st) {
        setBusy(false);
        if (!st.isValid() && st.result != LicenseStatus::Result::None) setMessage(st.message, true);
    });
    connect(this, &QDialog::rejected, qApp, &QApplication::quit);
}

void ActivationDialog::setBusy(bool busy) {
    m_btn->setEnabled(!busy);
    m_key->setEnabled(!busy);
    m_btn->setText(busy ? "Проверка…" : (m_key->text().isEmpty() && m_lic.hasStoredLicense() ? "Повторить проверку" : "Активировать"));
}

void ActivationDialog::setMessage(const QString& text, bool error) {
    m_msg->setObjectName(error ? "warn" : "muted");
    m_msg->style()->unpolish(m_msg);
    m_msg->style()->polish(m_msg);
    m_msg->setText(text);
}

void ActivationDialog::openFresh(const QString& message, bool error) {
    m_key->clear();
    setBusy(false);
    setMessage(message, error);
    show();
    raise();
    activateWindow();
    anim::fadeIn(this, 300);
}

void ActivationDialog::onActivate() {
    const QString k = m_key->text().trimmed().toUpper();
    if (k.isEmpty() && m_lic.hasStoredLicense()) {
        setBusy(true);
        setMessage("Проверяем лицензию…", false);
        m_lic.verifyStored();
        return;
    }
    static const QRegularExpression re("^HANDI-[A-Z0-9]{4}(-[A-Z0-9]{4}){3}$");
    if (!re.match(k).hasMatch()) {
        setMessage("Неверный формат ключа. Пример: HANDI-ABCD-EFGH-JKLM-NPQR", true);
        return;
    }
    setBusy(true);
    setMessage("Проверяем ключ…", false);
    m_lic.activate(k);
}

}
