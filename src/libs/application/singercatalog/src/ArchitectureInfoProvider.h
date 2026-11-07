// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_SINGERCATALOG_ARCHITECTUREINFOPROVIDER_H
#define DIFFSCOPE_SINGERCATALOG_ARCHITECTUREINFOPROVIDER_H

#include <QObject>
#include <QScopedPointer>
#include <QStringList>
#include <qqmlintegration.h>

#include <singercatalog/ArchitectureInfo.h>
#include <singercatalog/singercatalogglobal.h>

Q_MOC_INCLUDE(<singercatalog/SingerRegistry.h>)

namespace SingerCatalog {

    class SingerRegistry;
    class ArchitectureInfoProviderPrivate;

    /** @brief Tracks an architecture's metadata and singer IDs as its registry changes. */
    class SINGERCATALOG_EXPORT ArchitectureInfoProvider : public QObject {
        Q_OBJECT
        QML_ELEMENT
        Q_DECLARE_PRIVATE(ArchitectureInfoProvider)
        Q_PROPERTY(SingerRegistry *registry READ registry WRITE setRegistry NOTIFY registryChanged)
        Q_PROPERTY(QString architectureId READ architectureId WRITE setArchitectureId NOTIFY architectureIdChanged)
        Q_PROPERTY(ArchitectureInfo info READ info NOTIFY infoChanged)
        Q_PROPERTY(bool exists READ exists NOTIFY existsChanged)
        Q_PROPERTY(QStringList singerIds READ singerIds NOTIFY singerIdsChanged)

    public:
        /** @brief Constructs an unbound provider with an optional QObject @p parent. */
        explicit ArchitectureInfoProvider(QObject *parent = nullptr);
        /** @brief Destroys the provider. */
        ~ArchitectureInfoProvider() override;

        /** @brief Returns the borrowed registry, or nullptr if unbound or the registry was destroyed. */
        SingerRegistry *registry() const;
        /** @brief Binds to @p registry without transferring ownership, or clears the binding if nullptr. */
        void setRegistry(SingerRegistry *registry);

        /** @brief Returns the architecture ID to resolve. */
        QString architectureId() const;
        /** @brief Sets the architecture ID to resolve and updates the exposed metadata. */
        void setArchitectureId(const QString &architectureId);

        /** @brief Returns the resolved metadata, or default-constructed metadata if the architecture is unavailable. */
        ArchitectureInfo info() const;
        /** @brief Returns whether the architecture is registered in the bound registry. */
        bool exists() const;
        /** @brief Returns singer IDs in key order, or an empty list if the architecture is unavailable. */
        QStringList singerIds() const;

    Q_SIGNALS:
        /** @brief Signals a registry binding change, including registry destruction. */
        void registryChanged(SingerRegistry *registry);
        /** @brief Signals a change to the architecture ID to resolve. */
        void architectureIdChanged(const QString &architectureId);
        /** @brief Signals a change to the resolved metadata. */
        void infoChanged(const ArchitectureInfo &info);
        /** @brief Signals a change to architecture availability. */
        void existsChanged(bool exists);
        /** @brief Signals a change to the resolved singer IDs. */
        void singerIdsChanged(const QStringList &singerIds);

    private:
        QScopedPointer<ArchitectureInfoProviderPrivate> d_ptr;
    };

}

#endif // DIFFSCOPE_SINGERCATALOG_ARCHITECTUREINFOPROVIDER_H
