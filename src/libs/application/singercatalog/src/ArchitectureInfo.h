// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_SINGERCATALOG_ARCHITECTUREINFO_H
#define DIFFSCOPE_SINGERCATALOG_ARCHITECTUREINFO_H

#include <QJsonValue>
#include <QMap>
#include <QMetaType>
#include <QQmlComponent>
#include <QSharedDataPointer>
#include <QString>
#include <QVariant>

#include <singercatalog/singercatalogglobal.h>

namespace SingerCatalog {

    class ArchitectureInfoData;

    struct SINGERCATALOG_EXPORT ParameterInfo {
        Q_GADGET
        Q_PROPERTY(QString displayName MEMBER displayName)
        Q_PROPERTY(double defaultValue MEMBER defaultValue)
        Q_PROPERTY(double baselineValue MEMBER baselineValue)
        Q_PROPERTY(FillMode fillMode MEMBER fillMode)
        Q_PROPERTY(ValueType valueType MEMBER valueType)
        Q_PROPERTY(bool showDefaultValue MEMBER showDefaultValue)
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
        double defaultValue{0.0}; // Fallback for missing or invalid parameter values.
        double baselineValue{0.0}; // Reference line and curve-fill baseline.
        FillMode fillMode{NoFill};
        ValueType valueType{Absolute};
        bool showDefaultValue{false};
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

    SINGERCATALOG_EXPORT ParameterInfo pitchParameterInfo();
    SINGERCATALOG_EXPORT ParameterInfo transformParameterInfo();

    class SINGERCATALOG_EXPORT ArchitectureInfo {
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

Q_DECLARE_METATYPE(SingerCatalog::ParameterInfo)
Q_DECLARE_METATYPE(SingerCatalog::ArchitectureInfo::ParameterMap)
Q_DECLARE_METATYPE(SingerCatalog::ArchitectureInfo)

#endif // DIFFSCOPE_SINGERCATALOG_ARCHITECTUREINFO_H
