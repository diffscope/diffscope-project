#ifndef DIFFSCOPE_COREPLUGIN_ARCHITECTUREINFO_H
#define DIFFSCOPE_COREPLUGIN_ARCHITECTUREINFO_H

#include <QJsonValue>
#include <QMap>
#include <QMetaType>
#include <QQmlComponent>
#include <QSharedDataPointer>
#include <QString>

#include <coreplugin/coreglobal.h>

namespace Core {

    class ArchitectureInfoData;

    class CORE_EXPORT ArchitectureInfo {
        Q_GADGET
        Q_PROPERTY(QString name READ name WRITE setName)
        Q_PROPERTY(ParameterMap parameters READ parameters WRITE setParameters)
        Q_PROPERTY(QJsonValue defaultExtra READ defaultExtra WRITE setDefaultExtra)
        Q_PROPERTY(QQmlComponent *controlPanelComponent READ controlPanelComponent WRITE setControlPanelComponent)
    public:
        struct Parameter {
            // TODO

            bool operator==(const Parameter &) const = default;
            bool operator!=(const Parameter &) const = default;
        };

        using ParameterMap = QMap<QString, Parameter>;

        ArchitectureInfo();
        ArchitectureInfo(const ArchitectureInfo &other);
        ArchitectureInfo(ArchitectureInfo &&other) noexcept;
        ArchitectureInfo &operator=(const ArchitectureInfo &other);
        ArchitectureInfo &operator=(ArchitectureInfo &&other) noexcept;
        ~ArchitectureInfo();

        QString name() const;
        void setName(const QString &name);

        ParameterMap parameters() const;
        void setParameters(const ParameterMap &parameters);

        QJsonValue defaultExtra() const;
        void setDefaultExtra(const QJsonValue &defaultExtra);

        QQmlComponent *controlPanelComponent() const;
        void setControlPanelComponent(QQmlComponent *component);

        bool operator==(const ArchitectureInfo &other) const;
        bool operator!=(const ArchitectureInfo &other) const;

    private:
        QSharedDataPointer<ArchitectureInfoData> d;
    };

}

Q_DECLARE_METATYPE(Core::ArchitectureInfo::Parameter)
Q_DECLARE_METATYPE(Core::ArchitectureInfo::ParameterMap)
Q_DECLARE_METATYPE(Core::ArchitectureInfo)

#endif // DIFFSCOPE_COREPLUGIN_ARCHITECTUREINFO_H
