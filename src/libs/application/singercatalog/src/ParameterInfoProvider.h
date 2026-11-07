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

    /**
     * @brief Tracks parameter metadata and converts between normalized, display, and DSPX model values.
     *
     * The pitch parameter is available without a registry. Other parameters require a registered architecture.
     */
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
        /** @brief Constructs an unbound provider with an optional QObject @p parent. */
        explicit ParameterInfoProvider(QObject *parent = nullptr);
        /** @brief Destroys the provider. */
        ~ParameterInfoProvider() override;

        /** @brief Returns the borrowed registry, or nullptr if unbound or the registry was destroyed. */
        SingerRegistry *registry() const;
        /** @brief Binds to @p registry without transferring ownership, or clears the binding if nullptr. */
        void setRegistry(SingerRegistry *registry);

        /** @brief Returns the architecture ID used for parameter lookup. */
        QString architectureId() const;
        /** @brief Sets the architecture ID and updates the resolved parameter metadata. */
        void setArchitectureId(const QString &architectureId);

        /** @brief Returns the parameter ID to resolve. */
        QString parameterId() const;
        /** @brief Sets the parameter ID, with pitch selecting built-in pitch metadata. */
        void setParameterId(const QString &parameterId);

        /** @brief Returns whether available parameters use relative-adjustment metadata. Defaults to false. */
        bool isTransform() const;
        /** @brief Sets whether available parameters use transformParameterInfo() instead of their own metadata. */
        void setTransform(bool transform);

        /** @brief Returns the resolved metadata, or default-constructed metadata if the parameter is unavailable. */
        ParameterInfo info() const;
        /** @brief Returns whether the parameter is built-in pitch or is registered under the selected architecture. */
        bool exists() const;

        /**
         * @brief Returns the display value converted from @p value after QVariant::toDouble() and clamping to [0, 1].
         * Returns an invalid QVariant if the parameter is unavailable, the input is invalid, or the converted input is nonfinite.
         * @pre info().toDisplayValue is non-null if the parameter is available.
         */
        Q_INVOKABLE QVariant displayValue(const QVariant &value) const;
        /**
         * @brief Returns the display string formatted from @p value after QVariant::toDouble() and clamping to [0, 1].
         * Returns an empty string if the parameter is unavailable, the input is invalid, or the converted input is nonfinite.
         * @pre info().toDisplayString is non-null if the parameter is available.
         */
        Q_INVOKABLE QString displayString(const QVariant &value) const;
        /**
         * @brief Returns the normalized value converted from @p displayValue and clamped to [0, 1].
         * Returns an invalid QVariant if the parameter is unavailable or the callback result is nonfinite.
         * @pre info().fromDisplayValue is non-null if the parameter is available.
         */
        Q_INVOKABLE QVariant valueFromDisplay(double displayValue) const;
        /**
         * @brief Returns a normalized value using QVariant::toInt() and ParameterInfo::fromDspxModelValue().
         * Returns an invalid QVariant if the input is invalid. Parameter availability does not affect this conversion.
         */
        Q_INVOKABLE QVariant valueFromDspx(const QVariant &value) const;
        /**
         * @brief Returns a DSPX model integer using ParameterInfo::toDspxModelValue().
         *
         * Substitutes the metadata's clamped default value if @p value is nonfinite.
         * Parameter availability does not affect this conversion.
         */
        Q_INVOKABLE QVariant valueToDspx(double value) const;

    Q_SIGNALS:
        /** @brief Signals a registry binding change, including registry destruction. */
        void registryChanged(SingerRegistry *registry);
        /** @brief Signals a change to the architecture ID. */
        void architectureIdChanged(const QString &architectureId);
        /** @brief Signals a change to the parameter ID. */
        void parameterIdChanged(const QString &parameterId);
        /** @brief Signals a change to relative-adjustment mode. */
        void transformChanged(bool transform);
        /** @brief Signals a change to the resolved metadata. */
        void infoChanged(const ParameterInfo &info);
        /** @brief Signals a change to parameter availability. */
        void existsChanged(bool exists);

    private:
        QScopedPointer<ParameterInfoProviderPrivate> d_ptr;
    };

}

#endif // DIFFSCOPE_SINGERCATALOG_PARAMETERINFOPROVIDER_H
