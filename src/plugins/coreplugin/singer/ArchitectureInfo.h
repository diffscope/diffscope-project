// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#ifndef DIFFSCOPE_COREPLUGIN_ARCHITECTUREINFO_H
#define DIFFSCOPE_COREPLUGIN_ARCHITECTUREINFO_H

#include <QJsonValue>
#include <QMap>
#include <QMetaType>
#include <QQmlComponent>
#include <QSharedDataPointer>
#include <QString>
#include <QVariant>

#include <coreplugin/coreglobal.h>

namespace Core {

    class ArchitectureInfoData;

    struct CORE_EXPORT ParameterInfo {
        Q_GADGET
        Q_PROPERTY(QString displayName MEMBER displayName)
        Q_PROPERTY(double defaultValue MEMBER defaultValue)
        Q_PROPERTY(FillMode fillMode MEMBER fillMode)
        Q_PROPERTY(ValueType valueType MEMBER valueType)
        Q_PROPERTY(double divisionValue MEMBER divisionValue)
        Q_PROPERTY(bool showDefaultValue MEMBER showDefaultValue)
        Q_PROPERTY(bool showDivision MEMBER showDivision)
    public:
        enum FillMode {
            NoFill,
            TopFill,
            BottomFill,
            BaselineFill,
        };
        Q_ENUM(FillMode);

        enum ValueType {
            Absolute,
            Relative,
        };
        Q_ENUM(ValueType)

        QString displayName;
        double defaultValue{0.0};
        FillMode fillMode{NoFill};
        ValueType valueType{Absolute};
        double divisionValue{0.2};
        bool showDefaultValue{false};
        bool showDivision{true};
        QVariant userData;
        double (*toDisplayValue)(const ParameterInfo &, double){[](const ParameterInfo &, double value) {
            return value;
        }};
        double (*fromDisplayValue)(const ParameterInfo &, double){[](const ParameterInfo &, double value) {
            return value;
        }};
        QString (*toDisplayString)(const ParameterInfo &, double){[](const ParameterInfo &, double value) {
            return QString::number(value);
        }};

        Q_INVOKABLE double invokeToDisplayValue(double value) const {
            return toDisplayValue(*this, value);
        }
        Q_INVOKABLE double invokeFromDisplayValue(double value) const {
            return fromDisplayValue(*this, value);
        }
        Q_INVOKABLE QString invokeToDisplayString(double value) const {
            return toDisplayString(*this, value);
        }

        static double fromDspxModelValue(int value);
        static int toDspxModelValue(double value);

        bool operator==(const ParameterInfo &) const = default;
        bool operator!=(const ParameterInfo &) const = default;
    };

    CORE_EXPORT ParameterInfo pitchParameterInfo();
    CORE_EXPORT ParameterInfo transformParameterInfo();

    class CORE_EXPORT ArchitectureInfo {
        Q_GADGET
        Q_PROPERTY(QString name READ name WRITE setName)
        Q_PROPERTY(ParameterMap parameters READ parameters WRITE setParameters)
        Q_PROPERTY(QJsonValue defaultExtra READ defaultExtra WRITE setDefaultExtra)
        Q_PROPERTY(QQmlComponent *controlPanelComponent READ controlPanelComponent WRITE setControlPanelComponent)
    public:

        using ParameterMap = QMap<QString, ParameterInfo>;

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

Q_DECLARE_METATYPE(Core::ParameterInfo)
Q_DECLARE_METATYPE(Core::ArchitectureInfo::ParameterMap)
Q_DECLARE_METATYPE(Core::ArchitectureInfo)

#endif // DIFFSCOPE_COREPLUGIN_ARCHITECTUREINFO_H
