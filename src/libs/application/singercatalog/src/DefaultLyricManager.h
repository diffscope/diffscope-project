// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_SINGERCATALOG_DEFAULTLYRICMANAGER_H
#define DIFFSCOPE_SINGERCATALOG_DEFAULTLYRICMANAGER_H

#include <QObject>
#include <QScopedPointer>
#include <QString>
#include <qqmlintegration.h>

#include <singercatalog/singercatalogglobal.h>

namespace dspx {
    class SingingClip;
}

namespace SingerCatalog {

    class DefaultLyricManagerPrivate;
    class SingerRegistry;

    class SINGERCATALOG_EXPORT DefaultLyricManager : public QObject {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_DECLARE_PRIVATE(DefaultLyricManager)
        Q_PROPERTY(QString fallback READ fallback WRITE setFallback NOTIFY fallbackChanged)

    public:
        // The registry must be non-null and remain valid during lyric queries.
        explicit DefaultLyricManager(const SingerRegistry *singerRegistry, QObject *parent = nullptr);
        ~DefaultLyricManager() override;

        QString fallback() const;
        void setFallback(const QString &fallback);

        Q_INVOKABLE QString getDefaultLyricForSingingClip(dspx::SingingClip *targetSingingClip) const;

    Q_SIGNALS:
        void fallbackChanged();

    private:
        QScopedPointer<DefaultLyricManagerPrivate> d_ptr;
    };

}

#endif // DIFFSCOPE_SINGERCATALOG_DEFAULTLYRICMANAGER_H
