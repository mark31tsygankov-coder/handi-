#include "licensing/LicenseManager.h"
#include <QCryptographicHash>
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSaveFile>
#include <QSettings>
#include <QSysInfo>
#include <windows.h>
#include <wincrypt.h>
#include <dpapi.h>

namespace handi {

static constexpr qint64 kOfflineGraceSec = 72 * 3600;
static const QByteArray kEntropy = "HANDI-license-v1";

static QByteArray dpapiProtect(const QByteArray& in) {
    DATA_BLOB i{DWORD(in.size()), reinterpret_cast<BYTE*>(const_cast<char*>(in.data()))};
    DATA_BLOB e{DWORD(kEntropy.size()), reinterpret_cast<BYTE*>(const_cast<char*>(kEntropy.data()))};
    DATA_BLOB o{};
    if (!CryptProtectData(&i, L"HANDI license", &e, nullptr, nullptr, CRYPTPROTECT_UI_FORBIDDEN, &o)) return {};
    QByteArray r(reinterpret_cast<const char*>(o.pbData), qsizetype(o.cbData));
    LocalFree(o.pbData);
    return r;
}

static QByteArray dpapiUnprotect(const QByteArray& in) {
    DATA_BLOB i{DWORD(in.size()), reinterpret_cast<BYTE*>(const_cast<char*>(in.data()))};
    DATA_BLOB e{DWORD(kEntropy.size()), reinterpret_cast<BYTE*>(const_cast<char*>(kEntropy.data()))};
    DATA_BLOB o{};
    if (!CryptUnprotectData(&i, nullptr, &e, nullptr, nullptr, CRYPTPROTECT_UI_FORBIDDEN, &o)) return {};
    QByteArray r(reinterpret_cast<const char*>(o.pbData), qsizetype(o.cbData));
    LocalFree(o.pbData);
    return r;
}

LicenseManager::LicenseManager(const QUrl& apiBase, const QString& storagePath, QObject* parent)
    : QObject(parent), m_base(apiBase), m_path(storagePath) {
    qRegisterMetaType<handi::LicenseStatus>();
}

QString LicenseManager::deviceId() const {
    QSettings reg("HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Cryptography", QSettings::NativeFormat);
    QString guid = reg.value("MachineGuid").toString();
    if (guid.isEmpty()) guid = QSysInfo::machineUniqueId();
    if (guid.isEmpty()) guid = QSysInfo::machineHostName();
    return QString::fromLatin1(QCryptographicHash::hash(("HANDI|" + guid).toUtf8(), QCryptographicHash::Sha256).toHex());
}

QString LicenseManager::deviceName() const { return QSysInfo::machineHostName(); }

std::optional<LicenseManager::Stored> LicenseManager::load() const {
    QFile f(m_path);
    if (!f.open(QIODevice::ReadOnly)) return std::nullopt;
    const QByteArray plain = dpapiUnprotect(f.readAll());
    const QJsonObject o = QJsonDocument::fromJson(plain).object();
    if (o.value("key").toString().isEmpty() || o.value("device").toString() != deviceId()) return std::nullopt;
    Stored s;
    s.key = o.value("key").toString();
    s.lastOk = qint64(o.value("last_ok").toDouble());
    s.expires = o.value("expires").toString();
    s.lifetime = o.value("lifetime").toBool();
    return s;
}

bool LicenseManager::save(const Stored& s) const {
    const QJsonObject o{{"key", s.key}, {"last_ok", double(s.lastOk)}, {"expires", s.expires},
                        {"lifetime", s.lifetime}, {"device", deviceId()}};
    const QByteArray enc = dpapiProtect(QJsonDocument(o).toJson(QJsonDocument::Compact));
    if (enc.isEmpty()) return false;
    QDir().mkpath(QFileInfo(m_path).absolutePath());
    QSaveFile f(m_path);
    if (!f.open(QIODevice::WriteOnly)) return false;
    f.write(enc);
    return f.commit();
}

bool LicenseManager::hasStoredLicense() const { return load().has_value(); }

QString LicenseManager::storedKeyMasked() const {
    const auto s = load();
    if (!s) return {};
    return s->key.left(6) + "••••-••••-••••-" + s->key.right(4);
}

void LicenseManager::removeLicense() {
    QFile::remove(m_path);
    m_status = {};
}

void LicenseManager::finish(const LicenseStatus& s) {
    m_status = s;
    emit finished(s);
}

void LicenseManager::activate(const QString& key) {
    send("activate.php", key.trimmed().toUpper(), true);
}

void LicenseManager::verifyStored() {
    const auto s = load();
    if (!s) { finish(LicenseStatus{LicenseStatus::Result::None, "no_license", "Лицензия не активирована"}); return; }
    send("check.php", s->key, false);
}

LicenseStatus LicenseManager::offlineFallback(const Stored& s, const QString& netMsg) const {
    LicenseStatus st;
    const qint64 now = QDateTime::currentSecsSinceEpoch();
    const QDateTime exp = QDateTime::fromString(s.expires, Qt::ISODate);
    const bool notExpired = s.lifetime || (exp.isValid() && exp > QDateTime::currentDateTimeUtc());
    // now < lastOk — признак перевода часов назад
    if (notExpired && now >= s.lastOk && now - s.lastOk <= kOfflineGraceSec) {
        st.result = LicenseStatus::Result::Valid;
        st.offline = true;
        st.lifetime = s.lifetime;
        st.expiresAt = exp;
        st.message = "Нет связи с сервером, работа по сохранённой лицензии";
        return st;
    }
    st.result = LicenseStatus::Result::NetworkError;
    st.errorCode = "network";
    st.message = netMsg + ". Подключитесь к интернету для проверки лицензии";
    return st;
}

void LicenseManager::send(const QString& endpoint, const QString& key, bool activating) {
    if (m_base.scheme() != "https") {
        finish(LicenseStatus{LicenseStatus::Result::Invalid, "config", "Адрес сервера лицензий должен начинаться с https://"});
        return;
    }
    QUrl url = m_base;
    url.setPath(m_base.path() + "/" + endpoint);
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    req.setHeader(QNetworkRequest::UserAgentHeader, "HANDI/1.0");
    req.setTransferTimeout(15000);
    const QJsonObject body{{"key", key}, {"device_id", deviceId()}, {"device_name", deviceName()}};
    QNetworkReply* reply = m_net.post(req, QJsonDocument(body).toJson(QJsonDocument::Compact));

    connect(reply, &QNetworkReply::finished, this, [this, reply, key, activating] {
        reply->deleteLater();
        const QVariant http = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
        const QByteArray data = reply->readAll();
        const QJsonObject o = QJsonDocument::fromJson(data).object();

        if (!http.isValid() || o.isEmpty() || (http.toInt() >= 500) || http.toInt() == 429) {
            const QString netMsg = http.toInt() == 429 ? QString("Слишком много запросов") : QString("Сервер недоступен");
            // Диагностика: показываем, что реально ответил сервер (или почему нет ответа).
            const QString body = (o.isEmpty() && !data.isEmpty()) ? QStringLiteral(", не JSON: ") + QString::fromUtf8(data.left(70)).simplified() : QString();
            const QString detail = QStringLiteral(" [HTTP %1; %2%3]")
                .arg(http.isValid() ? QString::number(http.toInt()) : QStringLiteral("нет ответа"), reply->errorString(), body);
            if (!activating) {
                if (const auto s = load()) { finish(offlineFallback(*s, netMsg)); return; }
            }
            finish(LicenseStatus{LicenseStatus::Result::NetworkError, "network", netMsg + detail + ". Проверьте подключение к интернету"});
            return;
        }

        LicenseStatus st;
        if (o.value("ok").toBool()) {
            st.result = LicenseStatus::Result::Valid;
            st.lifetime = o.value("lifetime").toBool();
            if (!st.lifetime) st.expiresAt = QDateTime::fromString(o.value("expires_at").toString(), Qt::ISODate).toUTC();
            Stored s{key, QDateTime::currentSecsSinceEpoch(), st.lifetime ? QString() : st.expiresAt.toString(Qt::ISODate), st.lifetime};
            if (!save(s)) {
                st.result = LicenseStatus::Result::Invalid;
                st.errorCode = "storage";
                st.message = "Не удалось сохранить лицензию на этом компьютере";
            } else st.message = "Лицензия активна";
        } else {
            st.result = LicenseStatus::Result::Invalid;
            st.errorCode = o.value("error").toString();
            st.message = o.value("message").toString("Ключ не принят");
        }
        finish(st);
    });
}

} // namespace handi
