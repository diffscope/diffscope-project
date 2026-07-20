#ifndef DIFFSCOPE_COREPLUGIN_PARAMETERINFOPROVIDER_H
#define DIFFSCOPE_COREPLUGIN_PARAMETERINFOPROVIDER_H

#include <QObject>
#include <QScopedPointer>
#include <QVariant>
#include <qqmlintegration.h>

#include <coreplugin/ArchitectureInfo.h>
#include <coreplugin/coreglobal.h>

namespace Core {

    class ParameterInfoProviderPrivate;
    class SingerRegistry;

    class CORE_EXPORT ParameterInfoProvider : public QObject {
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

        Q_INVOKABLE QVariant displayValue(const QVariant &rawValue) const;
        Q_INVOKABLE QString displayString(const QVariant &rawValue) const;
        Q_INVOKABLE QVariant rawValue(double displayValue) const;

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

#endif // DIFFSCOPE_COREPLUGIN_PARAMETERINFOPROVIDER_H
