#include "TimeSignature.h"
#include "TimeSignature_p.h"
#include <QVariant>

#include <opendspx/timesignature.h>

#include <dspxmodel/Model.h>
#include <dspxmodel/ModelStrategy.h>
#include <dspxmodel/TimeSignatureSequence.h>

namespace dspx {

    static constexpr bool validateDenominator(int d) {
        return d == 1 || d == 2 || d == 4 || d == 8 || d == 16 || d == 32 || d == 64 || d == 128;
    }

    void TimeSignaturePrivate::setTimeSignatureSequence(TimeSignature *item, TimeSignatureSequence *timeSignatureSequence) {
        auto d = item->d_func();
        if (d->timeSignatureSequence != timeSignatureSequence) {
            d->timeSignatureSequence = timeSignatureSequence;
            Q_EMIT item->timeSignatureSequenceChanged();
        }
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
        model()->strategy()->setEntityProperty(handle(), ModelStrategy::P_Measure, index);
    }

    int TimeSignature::numerator() const {
        Q_D(const TimeSignature);
        return d->numerator;
    }

    void TimeSignature::setNumerator(int numerator) {
        Q_D(TimeSignature);
        Q_ASSERT(numerator > 0);
        model()->strategy()->setEntityProperty(handle(), ModelStrategy::P_Numerator, numerator);
    }

    int TimeSignature::denominator() const {
        Q_D(const TimeSignature);
        return d->denominator;
    }

    void TimeSignature::setDenominator(int denominator) {
        Q_D(TimeSignature);
        Q_ASSERT(validateDenominator(denominator));
        model()->strategy()->setEntityProperty(handle(), ModelStrategy::P_Denominator, denominator);
    }

    TimeSignatureSequence *TimeSignature::timeSignatureSequence() const {
        Q_D(const TimeSignature);
        return d->timeSignatureSequence;
    }

    QDspx::TimeSignature TimeSignature::toQDspx() const {
        return {
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
