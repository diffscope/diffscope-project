#include "TimeSignature.h"

#include <QVariant>
#include <QJSEngine>

#include <opendspx/timesignature.h>

#include <dspxmodel/Model.h>
#include <dspxmodel/ModelStrategy.h>

namespace dspx {

    class TimeSignaturePrivate {
        Q_DECLARE_PUBLIC(TimeSignature)
    public:
        TimeSignature *q_ptr;
        int index;
        int numerator;
        int denominator;

        void setIndexUnchecked(int index_);
        void setIndex(int index_);
        void setNumeratorUnchecked(int numerator_);
        void setNumerator(int numerator_);
        void setDenominatorUnchecked(int denominator_);
        void setDenominator(int denominator_);

    };

    void TimeSignaturePrivate::setIndexUnchecked(int index_) {
        Q_Q(TimeSignature);
        q->model()->strategy()->setEntityProperty(q->handle(), ModelStrategy::P_Measure, index_);
    }

    void TimeSignaturePrivate::setIndex(int index_) {
        Q_Q(TimeSignature);
        if (auto engine = qjsEngine(q); engine && index_ < 0) {
            engine->throwError(QJSValue::RangeError, QStringLiteral("Index must be greater or equal to 0"));
            return;
        }
        setIndexUnchecked(index_);
    }

    void TimeSignaturePrivate::setNumeratorUnchecked(int numerator_) {
        Q_Q(TimeSignature);
        q->model()->strategy()->setEntityProperty(q->handle(), ModelStrategy::P_Numerator, numerator_);
    }

    void TimeSignaturePrivate::setNumerator(int numerator_) {
        Q_Q(TimeSignature);
        if (auto engine = qjsEngine(q); engine && numerator_ < 1) {
            engine->throwError(QJSValue::RangeError, QStringLiteral("Numerator must be greater or equal to 1"));
            return;
        }
        setNumeratorUnchecked(numerator_);
    }

    void TimeSignaturePrivate::setDenominatorUnchecked(int denominator_) {
        Q_Q(TimeSignature);
        q->model()->strategy()->setEntityProperty(q->handle(), ModelStrategy::P_Denominator, denominator_);
    }

    static constexpr bool validateDenominator(int d) {
        return d == 1 || d == 2 || d == 4 || d == 8 || d == 16 || d == 32 || d == 64 || d == 128 || d == 256;
    }

    void TimeSignaturePrivate::setDenominator(int denominator_) {
        Q_Q(TimeSignature);
        if (auto engine = qjsEngine(q); engine) {
            if (!validateDenominator(denominator_)) {
                engine->throwError(QJSValue::RangeError, QStringLiteral("Denominator must be one of: 1, 2, 4, 8, 16, 32, 64, 128, 256"));
                return;
            }
        }
        setDenominatorUnchecked(denominator_);
    }

    TimeSignature::TimeSignature(Handle handle, Model *model) : EntityObject(handle, model), d_ptr(new TimeSignaturePrivate) {
        Q_D(TimeSignature);
        Q_ASSERT(model->strategy()->getEntityType(handle) == ModelStrategy::EI_TimeSignature);
        d->q_ptr = this;
        d->index = model->strategy()->getEntityProperty(handle, ModelStrategy::P_Measure).toInt();
        d->numerator = model->strategy()->getEntityProperty(handle, ModelStrategy::P_Numerator).toInt();
        d->denominator = model->strategy()->getEntityProperty(handle, ModelStrategy::P_Denominator).toInt();
    }

    TimeSignature::~TimeSignature() = default;

    int TimeSignature::index() const {
        Q_D(const TimeSignature);
        return d->index;
    }

    void TimeSignature::setIndex(int index) {
        Q_D(TimeSignature);
        Q_ASSERT(index >= 0);
        d->setIndexUnchecked(index);
    }

    int TimeSignature::numerator() const {
        Q_D(const TimeSignature);
        return d->numerator;
    }

    void TimeSignature::setNumerator(int numerator) {
        Q_D(TimeSignature);
        Q_ASSERT(numerator >= 1);
        d->setNumeratorUnchecked(numerator);
    }

    int TimeSignature::denominator() const {
        Q_D(const TimeSignature);
        return d->denominator;
    }

    void TimeSignature::setDenominator(int denominator) {
        Q_D(TimeSignature);
        Q_ASSERT(validateDenominator(denominator));
        d->setDenominatorUnchecked(denominator);
    }

    QDspx::TimeSignature TimeSignature::toQDspx() const {
        return QDspx::TimeSignature {
            .index = index(),
            .numerator = numerator(),
            .denominator = denominator(),
        };
    }

    void TimeSignature::fromQDspx(const QDspx::TimeSignature &timeSignature) {
        // TODO QDspx needs to rename these properties
        setIndex(timeSignature.index);
        setNumerator(timeSignature.numerator);
        setDenominator(timeSignature.denominator);
    }

    void TimeSignature::handleSetEntityProperty(int property, const QVariant &value) {
        Q_D(TimeSignature);
        switch (property) {
            case ModelStrategy::P_Measure: {
                d->index = value.toInt();
                Q_EMIT indexChanged(d->index);
                break;
            }
            case ModelStrategy::P_Numerator: {
                d->numerator = value.toInt();
                Q_EMIT numeratorChanged(d->numerator);
                break;
            }
            case ModelStrategy::P_Denominator: {
                d->denominator = value.toInt();
                Q_EMIT denominatorChanged(d->denominator);
                break;
            }
            default:
                Q_UNREACHABLE();
        }
    }

}

#include "moc_TimeSignature.cpp"