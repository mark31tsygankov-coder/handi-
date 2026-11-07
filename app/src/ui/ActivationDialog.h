#pragma once
#include "licensing/LicenseManager.h"
#include <QDialog>

class QLabel;
class QLineEdit;
class QPushButton;

namespace handi {

class ActivationDialog : public QDialog {
    Q_OBJECT
public:
    explicit ActivationDialog(LicenseManager& lic, QWidget* parent = nullptr);
    void openFresh(const QString& message = {}, bool error = false);

private:
    void onActivate();
    void setMessage(const QString& text, bool error);
    void setBusy(bool busy);

    LicenseManager& m_lic;
    QLineEdit* m_key;
    QPushButton* m_btn;
    QLabel* m_msg;
};

}
