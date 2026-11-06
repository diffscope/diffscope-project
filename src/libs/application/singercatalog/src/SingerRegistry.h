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

    class SINGERCATALOG_EXPORT SingerRegistry : public QObject {
        Q_OBJECT
        QML_ANONYMOUS
        Q_DECLARE_PRIVATE(SingerRegistry)
        Q_PROPERTY(QStringList architectureIds READ architectureIds NOTIFY architectureIdsChanged)

    public:
        explicit SingerRegistry(QObject *parent = nullptr);
        ~SingerRegistry() override;

        bool registerArchitecture(const QString &architectureId, const ArchitectureInfo &info);
        bool updateArchitecture(const QString &architectureId, const ArchitectureInfo &info);
        bool removeArchitecture(const QString &architectureId);
        bool containsArchitecture(const QString &architectureId) const;
        ArchitectureInfo architectureInfo(const QString &architectureId) const;
        QStringList architectureIds() const;

        bool registerSinger(const QString &architectureId, const QString &singerId, const SingerInfo &info);
        bool updateSinger(const QString &architectureId, const QString &singerId, const SingerInfo &info);
        bool removeSinger(const QString &architectureId, const QString &singerId);
        bool containsSinger(const QString &architectureId, const QString &singerId) const;
        SingerInfo singerInfo(const QString &architectureId, const QString &singerId) const;
        QStringList singerIds(const QString &architectureId) const;

    Q_SIGNALS:
        void refreshRequested();
        void architectureIdsChanged(const QStringList &architectureIds);
        void architectureRegistered(const QString &architectureId);
        void architectureUpdated(const QString &architectureId);
        void architectureRemoved(const QString &architectureId);
        void singerRegistered(const QString &architectureId, const QString &singerId);
        void singerUpdated(const QString &architectureId, const QString &singerId);
        void singerRemoved(const QString &architectureId, const QString &singerId);

    private:
        QScopedPointer<SingerRegistryPrivate> d_ptr;
    };

}

#endif // DIFFSCOPE_SINGERCATALOG_SINGERREGISTRY_H
