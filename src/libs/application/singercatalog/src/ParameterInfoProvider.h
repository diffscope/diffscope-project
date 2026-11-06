// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_SINGERCATALOG_PARAMETERINFOPROVIDER_H
#define DIFFSCOPE_SINGERCATALOG_PARAMETERINFOPROVIDER_H

#include <QObject>
#include <QScopedPointer>
#include <QVariant>
#include <qqmlintegration.h>

#include <singercatalog/ArchitectureInfo.h>
#include <singercatalog/singercatalogglobal.h>

Q_MOC_INCLUDE(<singercatalog/SingerRegistry.h>)

namespace SingerCatalog {

    class ParameterInfoProviderPrivate;
    class SingerRegistry;

    class SINGERCATALOG_EXPORT ParameterInfoProvider : public QObject {
        Q_OBJECT
        QML_ELEMENT
        Q_DECLARE_PRIVATE(ParameterInfoProvider)
        Q_PROPERTY(SingerRegistry *registry READ registry WRITE setRegistry NOTIFY registryChanged)
        Q_PROPERTY(QString architectureId READ architectureId WRITE setArchitectureId NOTIFY architectureIdChanged)
        Q_PROPERTY(QString parameterId READ parameterId WRITE setParameterId NOTIFY parameterIdChanged)
        Q_PROPERTY(bool transform READ isTransform WRITE setTransform NOTIFY transformChanged)
        Q_PROPERTY(ParameterInfo info READ info NOTIFY infoChanged)
        Q_PROPERTY(bool exists READ exists NOTIFY existsChanged)

    public:
        explicit ParameterInfoProvider(QObject *parent = nullptr);
        ~ParameterInfoProvider() override;

        SingerRegistry *registry() const;
        void setRegistry(SingerRegistry *registry);

        QString architectureId() const;
        void setArchitectureId(const QString &architectureId);

        QString parameterId() const;
        void setParameterId(const QString &parameterId);

        bool isTransform() const;
        void setTransform(bool transform);

        ParameterInfo info() const;
        bool exists() const;

        Q_INVOKABLE QVariant displayValue(const QVariant &value) const;
        Q_INVOKABLE QString displayString(const QVariant &value) const;
        Q_INVOKABLE QVariant valueFromDisplay(double displayValue) const;
        Q_INVOKABLE QVariant valueFromDspx(const QVariant &value) const;
        Q_INVOKABLE QVariant valueToDspx(double value) const;

    Q_SIGNALS:
        void registryChanged(SingerRegistry *registry);
        void architectureIdChanged(const QString &architectureId);
        void parameterIdChanged(const QString &parameterId);
        void transformChanged(bool transform);
        void infoChanged(const ParameterInfo &info);
        void existsChanged(bool exists);

    private:
        QScopedPointer<ParameterInfoProviderPrivate> d_ptr;
    };

}

#endif // DIFFSCOPE_SINGERCATALOG_PARAMETERINFOPROVIDER_H
