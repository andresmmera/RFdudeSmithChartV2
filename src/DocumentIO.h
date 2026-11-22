#pragma once

#include "RfModel.h"

#include <QByteArray>
#include <QString>

namespace documents {

QByteArray toJson(const rf::Document& document);
rf::Document fromJson(const QByteArray& data);
rf::Document fromLegacy(const QByteArray& data);
QByteArray componentToJson(const rf::Component& component);
rf::Component componentFromJson(const QByteArray& data);
rf::Document load(const QString& path, bool* imported = nullptr);
void save(const QString& path, const rf::Document& document);

}
