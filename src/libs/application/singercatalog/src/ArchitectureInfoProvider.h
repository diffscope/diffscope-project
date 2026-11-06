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
        explicit ArchitectureInfoProvider(QObject *parent = nullptr);
        ~ArchitectureInfoProvider() override;

        SingerRegistry *registry() const;
        void setRegistry(SingerRegistry *registry);

        QString architectureId() const;
        void setArchitectureId(const QString &architectureId);

        ArchitectureInfo info() const;
        bool exists() const;
        QStringList singerIds() const;

    Q_SIGNALS:
        void registryChanged(SingerRegistry *registry);
        void architectureIdChanged(const QString &architectureId);
        void infoChanged(const ArchitectureInfo &info);
        void existsChanged(bool exists);
        void singerIdsChanged(const QStringList &singerIds);

    private:
        QScopedPointer<ArchitectureInfoProviderPrivate> d_ptr;
    };

}

#endif // DIFFSCOPE_SINGERCATALOG_ARCHITECTUREINFOPROVIDER_H
