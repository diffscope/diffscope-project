#ifndef DIFFSCOPE_SYNTH_PARAMETERCONFIGURATION_H
#define DIFFSCOPE_SYNTH_PARAMETERCONFIGURATION_H

#include <QJsonObject>
#include <QMetaType>
#include <QSharedDataPointer>
#include <QString>
#include <QStringList>

#include <synth/synthglobal.h>

namespace Synth {

    class ParameterConfigurationData;

    class SYNTH_EXPORT ParameterConfiguration {
        Q_GADGET
        Q_PROPERTY(QString id READ id WRITE setId)
        Q_PROPERTY(QString architectureId READ architectureId WRITE setArchitectureId)
        Q_PROPERTY(QString displayName READ displayName WRITE setDisplayName)
        Q_PROPERTY(int minimumValue READ minimumValue WRITE setMinimumValue)
        Q_PROPERTY(int maximumValue READ maximumValue WRITE setMaximumValue)
        Q_PROPERTY(bool showDefaultValue READ showDefaultValue WRITE setShowDefaultValue)
        Q_PROPERTY(int defaultValue READ defaultValue WRITE setDefaultValue)
        Q_PROPERTY(FillMode fillMode READ fillMode WRITE setFillMode)
        Q_PROPERTY(ValueType valueType READ valueType WRITE setValueType)
        Q_PROPERTY(bool showDivision READ showDivision WRITE setShowDivision)
        Q_PROPERTY(int divisionValue READ divisionValue WRITE setDivisionValue)
        Q_PROPERTY(QString normalizationExpression READ normalizationExpression WRITE setNormalizationExpression)
        Q_PROPERTY(QString denormalizationExpression READ denormalizationExpression WRITE setDenormalizationExpression)
        Q_PROPERTY(QString displayValueExpression READ displayValueExpression WRITE setDisplayValueExpression)
        Q_PROPERTY(QString displayValueInverseExpression READ displayValueInverseExpression WRITE setDisplayValueInverseExpression)
        Q_PROPERTY(QString displayTextTemplate READ displayTextTemplate WRITE setDisplayTextTemplate)

    public:
        enum FillMode {
            NoFill,
            TopFill,
            BottomFill,
            BaselineFill,
        };
        Q_ENUM(FillMode)

        enum ValueType {
            Absolute,
            Relative,
        };
        Q_ENUM(ValueType)

        ParameterConfiguration();
        ParameterConfiguration(const ParameterConfiguration &other);
        ParameterConfiguration(ParameterConfiguration &&other) noexcept;
        ParameterConfiguration &operator=(const ParameterConfiguration &other);
        ParameterConfiguration &operator=(ParameterConfiguration &&other) noexcept;
        ~ParameterConfiguration();

        QString id() const;
        void setId(const QString &id);
        QString architectureId() const;
        void setArchitectureId(const QString &architectureId);
        QString displayName() const;
        void setDisplayName(const QString &name);
        int minimumValue() const;
        void setMinimumValue(int value);
        int maximumValue() const;
        void setMaximumValue(int value);
        bool showDefaultValue() const;
        void setShowDefaultValue(bool show);
        int defaultValue() const;
        void setDefaultValue(int value);
        FillMode fillMode() const;
        void setFillMode(FillMode mode);
        ValueType valueType() const;
        void setValueType(ValueType type);
        bool showDivision() const;
        void setShowDivision(bool show);
        int divisionValue() const;
        void setDivisionValue(int value);
        QString normalizationExpression() const;
        void setNormalizationExpression(const QString &expression);
        QString denormalizationExpression() const;
        void setDenormalizationExpression(const QString &expression);
        QString displayValueExpression() const;
        void setDisplayValueExpression(const QString &expression);
        QString displayValueInverseExpression() const;
        void setDisplayValueInverseExpression(const QString &expression);
        QString displayTextTemplate() const;
        void setDisplayTextTemplate(const QString &displayTemplate);

        bool validate(QStringList *errors = nullptr) const;
        QJsonObject toJson() const;
        static bool fromJson(const QJsonObject &object, ParameterConfiguration *result,
                             QString *errorMessage = nullptr);

        bool operator==(const ParameterConfiguration &other) const;
        bool operator!=(const ParameterConfiguration &other) const;

    private:
        QSharedDataPointer<ParameterConfigurationData> d;
    };

}

Q_DECLARE_METATYPE(Synth::ParameterConfiguration)

#endif // DIFFSCOPE_SYNTH_PARAMETERCONFIGURATION_H
