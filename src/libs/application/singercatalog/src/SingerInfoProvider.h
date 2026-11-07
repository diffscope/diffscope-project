// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_SINGERCATALOG_SINGERINFOPROVIDER_H
#define DIFFSCOPE_SINGERCATALOG_SINGERINFOPROVIDER_H

#include <QObject>
#include <QScopedPointer>
#include <QVariantList>
#include <qqmlintegration.h>

#include <singercatalog/SingerInfo.h>
#include <singercatalog/singercatalogglobal.h>

Q_MOC_INCLUDE(<singercatalog/SingerRegistry.h>)

namespace SingerCatalog {

    class SingerRegistry;
    class SingerInfoProviderPrivate;

    /** @brief Tracks a singer's metadata and language options as its registry changes. */
    class SINGERCATALOG_EXPORT SingerInfoProvider : public QObject {
        Q_OBJECT
        QML_ELEMENT
        Q_DECLARE_PRIVATE(SingerInfoProvider)
        Q_PROPERTY(SingerRegistry *registry READ registry WRITE setRegistry NOTIFY registryChanged)
        Q_PROPERTY(QString architectureId READ architectureId WRITE setArchitectureId NOTIFY architectureIdChanged)
        Q_PROPERTY(QString singerId READ singerId WRITE setSingerId NOTIFY singerIdChanged)
        Q_PROPERTY(SingerInfo info READ info NOTIFY infoChanged)
        Q_PROPERTY(QVariantList languageOptions READ languageOptions NOTIFY languageOptionsChanged)
        Q_PROPERTY(bool exists READ exists NOTIFY existsChanged)

    public:
        /** @brief Constructs an unbound provider with an optional QObject @p parent. */
        explicit SingerInfoProvider(QObject *parent = nullptr);
        /** @brief Destroys the provider. */
        ~SingerInfoProvider() override;

        /** @brief Returns the borrowed registry, or nullptr if unbound or the registry was destroyed. */
        SingerRegistry *registry() const;
        /** @brief Binds to @p registry without transferring ownership, or clears the binding if nullptr. */
        void setRegistry(SingerRegistry *registry);

        /** @brief Returns the architecture ID used to resolve the singer. */
        QString architectureId() const;
        /** @brief Sets the architecture ID and updates the resolved singer metadata. */
        void setArchitectureId(const QString &architectureId);

        /** @brief Returns the singer ID to resolve within the architecture. */
        QString singerId() const;
        /** @brief Sets the singer ID and updates the resolved metadata. */
        void setSingerId(const QString &singerId);

        /** @brief Returns the resolved metadata, or default-constructed metadata if the singer is unavailable. */
        SingerInfo info() const;
        /**
         * @brief Returns language options as QVariantMap entries with text and value keys.
         *
         * The first option has an empty value for the default language, followed by the explicit default language
         * if registered and then other languages in key order. An empty default language ID selects the first key.
         * Returns a single unavailable option with an empty value if no languages are registered.
         */
        QVariantList languageOptions() const;
        /** @brief Returns whether the singer is registered under the selected architecture in the bound registry. */
        bool exists() const;

    Q_SIGNALS:
        /** @brief Signals a registry binding change, including registry destruction. */
        void registryChanged(SingerRegistry *registry);
        /** @brief Signals a change to the architecture ID. */
        void architectureIdChanged(const QString &architectureId);
        /** @brief Signals a change to the singer ID. */
        void singerIdChanged(const QString &singerId);
        /** @brief Signals a change to the resolved metadata. */
        void infoChanged(const SingerInfo &info);
        /** @brief Signals that language options may have changed after a metadata change. */
        void languageOptionsChanged();
        /** @brief Signals a change to singer availability. */
        void existsChanged(bool exists);

    private:
        QScopedPointer<SingerInfoProviderPrivate> d_ptr;
    };

}

#endif // DIFFSCOPE_SINGERCATALOG_SINGERINFOPROVIDER_H
