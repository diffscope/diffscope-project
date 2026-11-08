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

    /** @brief Resolves default lyrics from singer metadata with a configurable fallback. */
    class SINGERCATALOG_EXPORT DefaultLyricManager : public QObject {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_DECLARE_PRIVATE(DefaultLyricManager)
        Q_PROPERTY(QString fallback READ fallback WRITE setFallback NOTIFY fallbackChanged)

    public:
        /**
         * @brief Constructs a lyric manager using a borrowed registry and an optional QObject @p parent.
         * @pre singerRegistry is non-null and remains valid during lyric queries.
         */
        explicit DefaultLyricManager(const SingerRegistry *singerRegistry, QObject *parent = nullptr);
        /** @brief Destroys the lyric manager. */
        ~DefaultLyricManager() override;

        /** @brief Returns the fallback lyric, which is initially empty. */
        QString fallback() const;
        /** @brief Sets the lyric returned if singer metadata provides no default lyric. */
        void setFallback(const QString &fallback);

        /**
         * @brief Returns the default lyric of the first single singer with a nonempty ID in depth-first list order.
         *
         * Uses the clip's default language if nonempty, otherwise the singer's default language.
         * Selects the first language in key order if both language IDs are empty.
         * Returns fallback() if sources, a matching singer, the selected language, or its default lyric is absent.
         * @pre targetSingingClip is non-null.
         */
        Q_INVOKABLE QString getDefaultLyricForSingingClip(dspx::SingingClip *targetSingingClip) const;

    Q_SIGNALS:
        /** @brief Signals a change to the fallback lyric. */
        void fallbackChanged();

    private:
        QScopedPointer<DefaultLyricManagerPrivate> d_ptr;
    };

}

#endif // DIFFSCOPE_SINGERCATALOG_DEFAULTLYRICMANAGER_H
