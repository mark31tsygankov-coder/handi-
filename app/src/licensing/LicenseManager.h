#pragma once
#include <QDateTime>
#include <QNetworkAccessManager>
#include <QObject>
#include <QUrl>
#include <optional>

namespace handi {

struct LicenseStatus {
    enum class Result { None, Valid, Invalid, NetworkError };
    Result result = Result::None;
    QString errorCode;       // blocked, expired, not_found, device_mismatch, ...
    QString message;         // текст для пользователя
    bool lifetime = false;
    QDateTime expiresAt;     // UTC; невалидна для бессрочной
    bool offline = false;    // валидна по сохранённым данным (сервер недоступен)

    bool isValid() const { return result == Result::Valid; }
};

class LicenseManager : public QObject {
    Q_OBJECT
public:
    // apiBase — https-адрес папки api; storagePath — файл для зашифрованной лицензии.
    LicenseManager(const QUrl& apiBase, const QString& storagePath, QObject* parent = nullptr);

    QString deviceId() const;     // SHA-256 от MachineGuid Windows
    QString deviceName() const;
    bool hasStoredLicense() const;
    QString storedKeyMasked() const;

    void activate(const QString& key);
    void verifyStored();
    void removeLicense();
    LicenseStatus status() const { return m_status; }

signals:
    void finished(const handi::LicenseStatus& status);

private:
    struct Stored { QString key; qint64 lastOk = 0; QString expires; bool lifetime = false; };
    void send(const QString& endpoint, const QString& key, bool activating);
    void finish(const LicenseStatus& s);
    std::optional<Stored> load() const;
    bool save(const Stored& s) const;
    LicenseStatus offlineFallback(const Stored& s, const QString& netMsg) const;

    QUrl m_base;
    QString m_path;
    QNetworkAccessManager m_net;
    LicenseStatus m_status;
};

} // namespace handi
Q_DECLARE_METATYPE(handi::LicenseStatus)
