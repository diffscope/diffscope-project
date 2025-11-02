#include "Vibrato.h"

#include <QJSEngine>
#include <QVariant>

#include <opendspx/vibrato.h>

#include <dspxmodel/ModelStrategy.h>
#include <dspxmodel/VibratoPointDataArray.h>
#include <dspxmodel/VibratoPoints.h>
#include <dspxmodel/private/Model_p.h>

namespace dspx {

    class VibratoPrivate {
        Q_DECLARE_PUBLIC(Vibrato)
    public:
        Vibrato *q_ptr;
        ModelPrivate *pModel;
        Handle handle;

        int amp;
        double end;
        double freq;
        int offset;
        double phase;
        VibratoPoints *points;
        double start;

        void setEndUnchecked(double end_);
        void setEnd(double end_);
        void setFreqUnchecked(double freq_);
        void setFreq(double freq_);
        void setPhaseUnchecked(double phase_);
        void setPhase(double phase_);
        void setStartUnchecked(double start_);
        void setStart(double start_);
    };

    void VibratoPrivate::setEndUnchecked(double end_) {
        Q_Q(Vibrato);
        pModel->strategy->setEntityProperty(handle, ModelStrategy::P_VibratoEnd, end_);
    }

    void VibratoPrivate::setEnd(double end_) {
        Q_Q(Vibrato);
        if (auto engine = qjsEngine(q); engine && (end_ < 0.0 || end_ > 1.0)) {
            engine->throwError(QJSValue::RangeError, QStringLiteral("End must be in range [0, 1]"));
            return;
        }
        setEndUnchecked(end_);
    }

    void VibratoPrivate::setFreqUnchecked(double freq_) {
        Q_Q(Vibrato);
        pModel->strategy->setEntityProperty(handle, ModelStrategy::P_VibratoFrequency, freq_);
    }

    void VibratoPrivate::setFreq(double freq_) {
        Q_Q(Vibrato);
        if (auto engine = qjsEngine(q); engine && freq_ < 0.0) {
            engine->throwError(QJSValue::RangeError, QStringLiteral("Freq must be greater than or equal to 0"));
            return;
        }
        setFreqUnchecked(freq_);
    }

    void VibratoPrivate::setPhaseUnchecked(double phase_) {
        Q_Q(Vibrato);
        pModel->strategy->setEntityProperty(handle, ModelStrategy::P_VibratoPhase, phase_);
    }

    void VibratoPrivate::setPhase(double phase_) {
        Q_Q(Vibrato);
        if (auto engine = qjsEngine(q); engine && (phase_ < 0.0 || phase_ > 1.0)) {
            engine->throwError(QJSValue::RangeError, QStringLiteral("Phase must be in range [0, 1]"));
            return;
        }
        setPhaseUnchecked(phase_);
    }

    void VibratoPrivate::setStartUnchecked(double start_) {
        Q_Q(Vibrato);
        pModel->strategy->setEntityProperty(handle, ModelStrategy::P_VibratoStart, start_);
    }

    void VibratoPrivate::setStart(double start_) {
        Q_Q(Vibrato);
        if (auto engine = qjsEngine(q); engine && (start_ < 0.0 || start_ > 1.0)) {
            engine->throwError(QJSValue::RangeError, QStringLiteral("Start must be in range [0, 1]"));
            return;
        }
        setStartUnchecked(start_);
    }

    Vibrato::Vibrato(Handle handle, Model *model) : QObject(model), d_ptr(new VibratoPrivate) {
        Q_D(Vibrato);
        d->q_ptr = this;
        d->pModel = ModelPrivate::get(model);
        d->handle = handle;
        d->amp = d->pModel->strategy->getEntityProperty(handle, ModelStrategy::P_VibratoAmplitude).toInt();
        d->end = d->pModel->strategy->getEntityProperty(handle, ModelStrategy::P_VibratoEnd).toDouble();
        d->freq = d->pModel->strategy->getEntityProperty(handle, ModelStrategy::P_VibratoFrequency).toDouble();
        d->offset = d->pModel->strategy->getEntityProperty(handle, ModelStrategy::P_VibratoOffset).toInt();
        d->phase = d->pModel->strategy->getEntityProperty(handle, ModelStrategy::P_VibratoPhase).toDouble();
        d->start = d->pModel->strategy->getEntityProperty(handle, ModelStrategy::P_VibratoStart).toDouble();
        d->points = d->pModel->createObject<VibratoPoints>(handle);
    }

    Vibrato::~Vibrato() = default;

    int Vibrato::amp() const {
        Q_D(const Vibrato);
        return d->amp;
    }

    void Vibrato::setAmp(int amp) {
        Q_D(Vibrato);
        d->pModel->strategy->setEntityProperty(d->handle, ModelStrategy::P_VibratoAmplitude, amp);
    }

    double Vibrato::end() const {
        Q_D(const Vibrato);
        return d->end;
    }

    void Vibrato::setEnd(double end) {
        Q_D(Vibrato);
        Q_ASSERT(end >= 0.0 && end <= 1.0);
        d->setEndUnchecked(end);
    }

    double Vibrato::freq() const {
        Q_D(const Vibrato);
        return d->freq;
    }

    void Vibrato::setFreq(double freq) {
        Q_D(Vibrato);
        Q_ASSERT(freq >= 0.0);
        d->setFreqUnchecked(freq);
    }

    int Vibrato::offset() const {
        Q_D(const Vibrato);
        return d->offset;
    }

    void Vibrato::setOffset(int offset) {
        Q_D(Vibrato);
        d->pModel->strategy->setEntityProperty(d->handle, ModelStrategy::P_VibratoOffset, offset);
    }

    double Vibrato::phase() const {
        Q_D(const Vibrato);
        return d->phase;
    }

    void Vibrato::setPhase(double phase) {
        Q_D(Vibrato);
        Q_ASSERT(phase >= 0.0 && phase <= 1.0);
        d->setPhaseUnchecked(phase);
    }

    VibratoPoints *Vibrato::points() const {
        Q_D(const Vibrato);
        return d->points;
    }

    double Vibrato::start() const {
        Q_D(const Vibrato);
        return d->start;
    }

    void Vibrato::setStart(double start) {
        Q_D(Vibrato);
        Q_ASSERT(start >= 0.0 && start <= 1.0);
        d->setStartUnchecked(start);
    }

    QDspx::Vibrato Vibrato::toQDspx() const {
        return {
            .start = start(),
            .end = end(),
            .amp = amp(),
            .freq = freq(),
            .phase = phase(),
            .offset = offset(),
            .points = points()->toQDspx(),
        };
    }

    void Vibrato::fromQDspx(const QDspx::Vibrato &vibrato) {
        setStart(vibrato.start);
        setEnd(vibrato.end);
        setAmp(vibrato.amp);
        setFreq(vibrato.freq);
        setPhase(vibrato.phase);
        setOffset(vibrato.offset);
        points()->fromQDspx(vibrato.points);
    }

    void Vibrato::handleProxySetEntityProperty(int property, const QVariant &value) {
        Q_D(Vibrato);
        switch (property) {
            case ModelStrategy::P_VibratoAmplitude: {
                d->amp = value.toInt();
                Q_EMIT ampChanged(d->amp);
                break;
            }
            case ModelStrategy::P_VibratoEnd: {
                d->end = value.toDouble();
                Q_EMIT endChanged(d->end);
                break;
            }
            case ModelStrategy::P_VibratoFrequency: {
                d->freq = value.toDouble();
                Q_EMIT freqChanged(d->freq);
                break;
            }
            case ModelStrategy::P_VibratoOffset: {
                d->offset = value.toInt();
                Q_EMIT offsetChanged(d->offset);
                break;
            }
            case ModelStrategy::P_VibratoPhase: {
                d->phase = value.toDouble();
                Q_EMIT phaseChanged(d->phase);
                break;
            }
            case ModelStrategy::P_VibratoStart: {
                d->start = value.toDouble();
                Q_EMIT startChanged(d->start);
                break;
            }
            default:
                Q_UNREACHABLE();
        }
    }

}

#include "moc_Vibrato.cpp"
