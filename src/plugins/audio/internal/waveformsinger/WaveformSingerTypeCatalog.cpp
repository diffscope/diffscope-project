#include "WaveformSingerTypeCatalog.h"

#include <QCoreApplication>
#include <QVariantMap>

namespace Audio::Internal {

    WaveformSingerTypeCatalog::WaveformSingerTypeCatalog(QObject *parent) : QObject(parent) {
    }

    WaveformSingerTypeCatalog::~WaveformSingerTypeCatalog() = default;

    QVariantList WaveformSingerTypeCatalog::entries() const {
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

    const QList<WaveformSingerTypeCatalog::Entry> &WaveformSingerTypeCatalog::availableTypes() {
        static const QList<Entry> entries{
            {QStringLiteral("piano"), QCoreApplication::translate("Audio::Internal::WaveformSingerTypeCatalog", "Electric Piano")},
            {QStringLiteral("sinewave"), QCoreApplication::translate("Audio::Internal::WaveformSingerTypeCatalog", "Sine Wave")},
            {QStringLiteral("choir"), QCoreApplication::translate("Audio::Internal::WaveformSingerTypeCatalog", "Choir")},
        };
        return entries;
    }

    QString WaveformSingerTypeCatalog::fallbackType() {
        return availableTypes().constFirst().type;
    }

    QString WaveformSingerTypeCatalog::normalizedType(const QString &type) {
        return availableTypes().at(indexOf(type)).type;
    }

    int WaveformSingerTypeCatalog::indexOf(const QString &type) {
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
