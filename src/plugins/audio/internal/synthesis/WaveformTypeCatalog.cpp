// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "WaveformTypeCatalog.h"

#include <QCoreApplication>
#include <QVariantMap>

namespace Audio::Internal {

    WaveformTypeCatalog::WaveformTypeCatalog(QObject *parent) : QObject(parent) {
    }

    WaveformTypeCatalog::~WaveformTypeCatalog() = default;

    QVariantList WaveformTypeCatalog::entries() const {
        QVariantList result;
        result.reserve(availableTypes().size());
        for (const auto &entry : availableTypes()) {
            result.append(QVariantMap{
                {QStringLiteral("type"), entry.type},
                {QStringLiteral("name"), entry.name},
            });
        }
        return result;
    }

    const QList<WaveformTypeCatalog::Entry> &WaveformTypeCatalog::availableTypes() {
        static const QList<Entry> entries{
            {QStringLiteral("piano"), QCoreApplication::translate("Audio::Internal::WaveformTypeCatalog", "Electric Piano")},
            {QStringLiteral("sinewave"), QCoreApplication::translate("Audio::Internal::WaveformTypeCatalog", "Sine Wave")},
            {QStringLiteral("choir"), QCoreApplication::translate("Audio::Internal::WaveformTypeCatalog", "Choir")},
        };
        return entries;
    }

    QString WaveformTypeCatalog::fallbackType() {
        return availableTypes().constFirst().type;
    }

    QString WaveformTypeCatalog::normalizedType(const QString &type) {
        return availableTypes().at(indexOf(type)).type;
    }

    int WaveformTypeCatalog::indexOf(const QString &type) {
        int index = 0;
        for (const auto &entry : availableTypes()) {
            if (entry.type == type) {
                return index;
            }
            ++index;
        }
        return 0;
    }

}
