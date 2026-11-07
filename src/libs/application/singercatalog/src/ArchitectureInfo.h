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

    /**
     * @brief Describes a parameter in the normalized range [0, 1] and its display conversions.
     *
     * Conversion callbacks receive this metadata and must be non-null when invoked.
     */
    struct SINGERCATALOG_EXPORT ParameterInfo {
        Q_GADGET
        Q_PROPERTY(QString displayName MEMBER displayName)
        Q_PROPERTY(double defaultValue MEMBER defaultValue)
        Q_PROPERTY(double baselineValue MEMBER baselineValue)
        Q_PROPERTY(FillMode fillMode MEMBER fillMode)
        Q_PROPERTY(ValueType valueType MEMBER valueType)
        Q_PROPERTY(bool showDefaultValue MEMBER showDefaultValue)
    public:
        /** @brief Specifies the area filled between a parameter curve and a reference. */
        enum FillMode {
            NoFill, ///< Disables curve filling.
            TopFill, ///< Fills toward the upper value limit.
            BottomFill, ///< Fills toward the lower value limit.
            BaselineFill, ///< Fills toward baselineValue.
        };
        Q_ENUM(FillMode);

        /** @brief Specifies the interpretation of parameter values. */
        enum ValueType {
            Absolute, ///< Represents an absolute value.
            Relative, ///< Represents a relative adjustment.
        };
        Q_ENUM(ValueType)

        QString displayName; ///< Contains the user-visible parameter name.
        double defaultValue{0.0}; ///< Specifies the normalized fallback for missing or invalid values.
        double baselineValue{0.0}; ///< Specifies the normalized reference line and curve-fill baseline.
        FillMode fillMode{NoFill}; ///< Specifies the curve-fill mode.
        ValueType valueType{Absolute}; ///< Specifies the value interpretation.
        bool showDefaultValue{false}; ///< Specifies whether the editor displays the reference line.
        QVariant userData; ///< Contains application-specific metadata available to conversion callbacks.
        /** @brief Converts a normalized value to a display value. Defaults to the identity conversion. */
        double (*toDisplayValue)(const ParameterInfo &, double){[](const ParameterInfo &, double value) {
            return value;
        }};
        /** @brief Converts a display value to a normalized value. Defaults to the identity conversion. */
        double (*fromDisplayValue)(const ParameterInfo &, double){[](const ParameterInfo &, double value) {
            return value;
        }};
        /** @brief Formats a normalized value for display. Defaults to QString::number(). */
        QString (*toDisplayString)(const ParameterInfo &, double){[](const ParameterInfo &, double value) {
            return QString::number(value);
        }};

        /**
         * @brief Returns the display value converted from @p value without clamping.
         * @pre toDisplayValue is non-null.
         */
        Q_INVOKABLE double invokeToDisplayValue(double value) const {
            return toDisplayValue(*this, value);
        }
        /**
         * @brief Returns the normalized value converted from @p value without clamping.
         * @pre fromDisplayValue is non-null.
         */
        Q_INVOKABLE double invokeFromDisplayValue(double value) const {
            return fromDisplayValue(*this, value);
        }
        /**
         * @brief Returns the display string formatted from normalized @p value without clamping.
         * @pre toDisplayString is non-null.
         */
        Q_INVOKABLE QString invokeToDisplayString(double value) const {
            return toDisplayString(*this, value);
        }

        /** @brief Returns @p value divided by 8064000 and clamped to [0, 1]. */
        static double fromDspxModelValue(int value);
        /** @brief Returns @p value clamped to [0, 1], scaled by 8064000, and rounded. Returns zero for NaN. */
        static int toDspxModelValue(double value);

        /** @brief Returns whether all members, including callback pointers, are equal. */
        bool operator==(const ParameterInfo &) const = default;
        /** @brief Returns whether any member, including callback pointers, differs. */
        bool operator!=(const ParameterInfo &) const = default;
    };

    /** @brief Returns pitch metadata with display values in [0, 128] and a normalized default of zero. */
    SINGERCATALOG_EXPORT ParameterInfo pitchParameterInfo();
    /** @brief Returns relative-adjustment metadata with display values in [0, 2] and a normalized default and baseline of 0.5. */
    SINGERCATALOG_EXPORT ParameterInfo transformParameterInfo();

    /** @brief Stores implicitly shared metadata for a singer architecture. */
    class SINGERCATALOG_EXPORT ArchitectureInfo {
        Q_GADGET
        Q_PROPERTY(QString name READ name WRITE setName)
        Q_PROPERTY(ParameterMap parameters READ parameters WRITE setParameters)
        Q_PROPERTY(QJsonValue defaultExtra READ defaultExtra WRITE setDefaultExtra)
        Q_PROPERTY(QQmlComponent *controlPanelComponent READ controlPanelComponent WRITE setControlPanelComponent)
    public:

        /** @brief Maps parameter identifiers to their metadata. */
        using ParameterMap = QMap<QString, ParameterInfo>;

        /** @brief Constructs empty metadata with no control panel component. */
        ArchitectureInfo();
        /** @brief Constructs a shared copy of @p other. */
        ArchitectureInfo(const ArchitectureInfo &other);
        /** @brief Moves metadata from @p other. */
        ArchitectureInfo(ArchitectureInfo &&other) noexcept;
        /** @brief Assigns a shared copy of @p other and returns this object. */
        ArchitectureInfo &operator=(const ArchitectureInfo &other);
        /** @brief Moves metadata from @p other and returns this object. */
        ArchitectureInfo &operator=(ArchitectureInfo &&other) noexcept;
        /** @brief Releases the shared metadata. */
        ~ArchitectureInfo();

        /** @brief Returns the user-visible architecture name. */
        QString name() const;
        /** @brief Sets the user-visible architecture name. */
        void setName(const QString &name);

        /** @brief Returns the supported parameter metadata indexed by parameter identifier. */
        ParameterMap parameters() const;
        /** @brief Sets the supported parameter metadata indexed by parameter identifier. */
        void setParameters(const ParameterMap &parameters);

        /** @brief Returns the default architecture-specific extra data. */
        QJsonValue defaultExtra() const;
        /** @brief Sets the default architecture-specific extra data. */
        void setDefaultExtra(const QJsonValue &defaultExtra);

        /** @brief Returns the borrowed control panel component, or nullptr if absent. */
        QQmlComponent *controlPanelComponent() const;
        /**
         * @brief Sets the control panel component, or clears it if @p component is nullptr.
         *
         * Ownership is not transferred. A non-null component must remain valid while referenced by any metadata copy.
         */
        void setControlPanelComponent(QQmlComponent *component);

        /** @brief Returns whether all metadata fields, including the component pointer, are equal. */
        bool operator==(const ArchitectureInfo &other) const;
        /** @brief Returns whether any metadata field, including the component pointer, differs. */
        bool operator!=(const ArchitectureInfo &other) const;

    private:
        QSharedDataPointer<ArchitectureInfoData> d;
    };

}

Q_DECLARE_METATYPE(SingerCatalog::ParameterInfo)
Q_DECLARE_METATYPE(SingerCatalog::ArchitectureInfo::ParameterMap)
Q_DECLARE_METATYPE(SingerCatalog::ArchitectureInfo)

#endif // DIFFSCOPE_SINGERCATALOG_ARCHITECTUREINFO_H
