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
        explicit SingerInfoProvider(QObject *parent = nullptr);
        ~SingerInfoProvider() override;

        SingerRegistry *registry() const;
        void setRegistry(SingerRegistry *registry);

        QString architectureId() const;
        void setArchitectureId(const QString &architectureId);

        QString singerId() const;
        void setSingerId(const QString &singerId);

        SingerInfo info() const;
        QVariantList languageOptions() const;
        bool exists() const;

    Q_SIGNALS:
        void registryChanged(SingerRegistry *registry);
        void architectureIdChanged(const QString &architectureId);
        void singerIdChanged(const QString &singerId);
        void infoChanged(const SingerInfo &info);
        void languageOptionsChanged();
        void existsChanged(bool exists);

    private:
        QScopedPointer<SingerInfoProviderPrivate> d_ptr;
    };

}

#endif // DIFFSCOPE_SINGERCATALOG_SINGERINFOPROVIDER_H
