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

    struct CORE_EXPORT ParameterInfo {
        Q_GADGET
        Q_PROPERTY(QString displayName MEMBER displayName)
        Q_PROPERTY(int bottomValue MEMBER bottomValue)
        Q_PROPERTY(int topValue MEMBER topValue)
        Q_PROPERTY(int defaultValue MEMBER defaultValue)
        Q_PROPERTY(FillMode fillMode MEMBER fillMode)
        Q_PROPERTY(ValueType valueType MEMBER valueType)
        Q_PROPERTY(int divisionValue MEMBER divisionValue)
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
        int bottomValue{0};
        int topValue{1000};
        int defaultValue{0};
        FillMode fillMode{NoFill};
        ValueType valueType{Absolute};
        int divisionValue{200};
        bool showDefaultValue{false};
        bool showDivision{true};
        double (*normalize)(const ParameterInfo &, int){[](const ParameterInfo &self, int value) {
            return static_cast<double>(value - self.bottomValue) / static_cast<double>(self.topValue - self.bottomValue);
        }};
        int (*denormalize)(const ParameterInfo &, double){[](const ParameterInfo &self, double value) {
            return static_cast<int>(value * static_cast<double>(self.topValue - self.bottomValue) + self.bottomValue);
        }};
        double (*toDisplayValue)(const ParameterInfo &, int){[](const ParameterInfo &self, int value) {
            return static_cast<double>(value);
        }};
        int (*fromDisplayValue)(const ParameterInfo &, double){[](const ParameterInfo &self, double value) {
            return static_cast<int>(value);
        }};
        QString (*toDisplayString)(const ParameterInfo &, int){[](const ParameterInfo &self, int value) {
            return QString::number(value);
        }};

        Q_INVOKABLE double invokeNormalize(int value) const {
            return normalize(*this, value);
        }
        Q_INVOKABLE int invokeDenormalize(double value) const {
            return denormalize(*this, value);
        }
        Q_INVOKABLE double invokeToDisplayValue(int value) const {
            return toDisplayValue(*this, value);
        }
        Q_INVOKABLE int invokeFromDisplayValue(double value) const {
            return fromDisplayValue(*this, value);
        }
        Q_INVOKABLE QString invokeToDisplayString(int value) const {
            return toDisplayString(*this, value);
        }

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
