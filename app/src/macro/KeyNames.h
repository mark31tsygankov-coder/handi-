#pragma once
#include <QList>
#include <QPair>
#include <QString>

namespace handi::keys {
// Имя клавиши по Windows virtual-key коду (пустая строка, если код неизвестен).
QString name(quint32 vk);
// Код по имени (без учёта регистра), 0 если не найдено.
quint32 fromName(const QString& name);
// Полный список поддерживаемых клавиш (для выпадающих списков).
const QList<QPair<quint32, QString>>& all();
}
