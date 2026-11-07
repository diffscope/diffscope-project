// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_SINGERCATALOG_SINGERREGISTRY_H
#define DIFFSCOPE_SINGERCATALOG_SINGERREGISTRY_H

#include <QObject>
#include <QScopedPointer>
#include <QStringList>
#include <qqmlintegration.h>

#include <singercatalog/ArchitectureInfo.h>
#include <singercatalog/SingerInfo.h>
#include <singercatalog/singercatalogglobal.h>

namespace SingerCatalog {

    class SingerRegistryPrivate;

    /**
     * @brief Stores architecture metadata and each architecture's singer metadata.
     *
     * Identifiers must be nonempty for registration. Singer identifiers are scoped to an architecture.
     * Metadata is stored by value. Referenced QObject ownership is not transferred.
     */
    class SINGERCATALOG_EXPORT SingerRegistry : public QObject {
        Q_OBJECT
        QML_ANONYMOUS
        Q_DECLARE_PRIVATE(SingerRegistry)
        Q_PROPERTY(QStringList architectureIds READ architectureIds NOTIFY architectureIdsChanged)

    public:
        /** @brief Constructs an empty registry with an optional QObject @p parent. */
        explicit SingerRegistry(QObject *parent = nullptr);
        /** @brief Destroys the registry and releases its metadata. */
        ~SingerRegistry() override;

        /** @brief Registers @p info and returns true, or returns false if @p architectureId is empty or already registered. */
        bool registerArchitecture(const QString &architectureId, const ArchitectureInfo &info);
        /** @brief Replaces architecture metadata and returns true, including for unchanged metadata. Returns false if the ID is unregistered. */
        bool updateArchitecture(const QString &architectureId, const ArchitectureInfo &info);
        /**
         * @brief Removes an architecture and its singers and returns true, or returns false if the ID is unregistered.
         *
         * Emits singerRemoved() for each singer before architectureRemoved() and architectureIdsChanged().
         */
        bool removeArchitecture(const QString &architectureId);
        /** @brief Returns whether @p architectureId identifies a registered architecture. */
        bool containsArchitecture(const QString &architectureId) const;
        /** @brief Returns the architecture metadata, or default-constructed metadata if the ID is unregistered. */
        ArchitectureInfo architectureInfo(const QString &architectureId) const;
        /** @brief Returns the registered architecture IDs in key order. */
        QStringList architectureIds() const;

        /**
         * @brief Registers singer metadata and returns true.
         * Returns false if either ID is empty, the architecture is unregistered, or the singer is already registered.
         */
        bool registerSinger(const QString &architectureId, const QString &singerId, const SingerInfo &info);
        /** @brief Replaces singer metadata and returns true, including for unchanged metadata. Returns false if either ID is unregistered. */
        bool updateSinger(const QString &architectureId, const QString &singerId, const SingerInfo &info);
        /** @brief Removes a singer and returns true, or returns false if either ID is unregistered. */
        bool removeSinger(const QString &architectureId, const QString &singerId);
        /** @brief Returns whether @p singerId is registered under @p architectureId. */
        bool containsSinger(const QString &architectureId, const QString &singerId) const;
        /** @brief Returns the singer metadata, or default-constructed metadata if either ID is unregistered. */
        SingerInfo singerInfo(const QString &architectureId, const QString &singerId) const;
        /** @brief Returns the architecture's singer IDs in key order, or an empty list if the architecture is unregistered. */
        QStringList singerIds(const QString &architectureId) const;

    Q_SIGNALS:
        /** @brief Requests a refresh by external metadata providers. Does not modify the registry directly. */
        void refreshRequested();
        /** @brief Signals a change to the registered architecture IDs and provides the new list. */
        void architectureIdsChanged(const QStringList &architectureIds);
        /** @brief Signals registration of an architecture. */
        void architectureRegistered(const QString &architectureId);
        /** @brief Signals a change to registered architecture metadata. */
        void architectureUpdated(const QString &architectureId);
        /** @brief Signals removal of an architecture and its singers. */
        void architectureRemoved(const QString &architectureId);
        /** @brief Signals registration of a singer under an architecture. */
        void singerRegistered(const QString &architectureId, const QString &singerId);
        /** @brief Signals a change to registered singer metadata. */
        void singerUpdated(const QString &architectureId, const QString &singerId);
        /** @brief Signals removal of a singer, including removal with its architecture. */
        void singerRemoved(const QString &architectureId, const QString &singerId);

    private:
        QScopedPointer<SingerRegistryPrivate> d_ptr;
    };

}

#endif // DIFFSCOPE_SINGERCATALOG_SINGERREGISTRY_H
