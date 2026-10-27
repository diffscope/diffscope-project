// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_AUDIO_WAVEFORMTYPECATALOG_H
#define DIFFSCOPE_AUDIO_WAVEFORMTYPECATALOG_H

#include <QList>
#include <QObject>
#include <QString>
#include <QVariantList>
#include <qqmlintegration.h>

class QQmlEngine;
class QJSEngine;

namespace Audio::Internal {

    class WaveformTypeCatalog : public QObject {
        Q_OBJECT
        QML_ELEMENT
        QML_SINGLETON
        Q_PROPERTY(QVariantList entries READ entries CONSTANT)

    public:
        struct Entry {
            QString type;
            QString name;
        };

        explicit WaveformTypeCatalog(QObject *parent = nullptr);
        ~WaveformTypeCatalog() override;

        static WaveformTypeCatalog *create(QQmlEngine *, QJSEngine *) {
            return new WaveformTypeCatalog;
        }

        QVariantList entries() const;

        static const QList<Entry> &availableTypes();
        static QString fallbackType();
        static QString normalizedType(const QString &type);
        static int indexOf(const QString &type);
    };

}

#endif // DIFFSCOPE_AUDIO_WAVEFORMTYPECATALOG_H
