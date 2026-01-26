#include "Tempo.h"
#include "Tempo_p.h"
#include <QVariant>

#include <opendspx/tempo.h>

#include <dspxmodel/Model.h>
#include <dspxmodel/ModelStrategy.h>
#include <dspxmodel/TempoSequence.h>

namespace dspx {

    void TempoPrivate::setTempoSequence(Tempo *item, TempoSequence *tempoSequence) {
        auto d = item->d_func();
        if (d->tempoSequence != tempoSequence) {
            d->tempoSequence = tempoSequence;
            Q_EMIT item->tempoSequenceChanged();
        }
    }

    Tempo::Tempo(Handle handle, Model *model) : EntityObject(handle, model), d_ptr(new TempoPrivate) {
        Q_D(Tempo);
        Q_ASSERT(model->strategy()->getEntityType(handle) == ModelStrategy::EI_Tempo);
        d->q_ptr = this;
        d->pos = model->strategy()->getEntityProperty(handle, ModelStrategy::P_Position).toInt();
        d->value = model->strategy()->getEntityProperty(handle, ModelStrategy::P_Value).toDouble();
    }

    Tempo::~Tempo() = default;

    int Tempo::pos() const {
        Q_D(const Tempo);
        return d->pos;
    }

    void Tempo::setPos(int pos) {
        Q_D(Tempo);
        Q_ASSERT(pos >= 0);
        model()->strategy()->setEntityProperty(handle(), ModelStrategy::P_Position, pos);
    }

    double Tempo::value() const {
        Q_D(const Tempo);
        return d->value;
    }

    void Tempo::setValue(double value) {
        Q_D(Tempo);
        Q_ASSERT(value >= 10.0 && value <= 1000.0);
        model()->strategy()->setEntityProperty(handle(), ModelStrategy::P_Value, value);
    }

    TempoSequence *Tempo::tempoSequence() const {
        Q_D(const Tempo);
        return d->tempoSequence;
    }

    QDspx::Tempo Tempo::toQDspx() const {
        return {
            .pos = pos(),
            .value = value(),
        };
    }

    void Tempo::fromQDspx(const QDspx::Tempo &tempo) {
        setPos(tempo.pos);
        setValue(tempo.value);
    }

    void Tempo::handleSetEntityProperty(int property, const QVariant &value) {
        Q_D(Tempo);
        switch (property) {
            case ModelStrategy::P_Position: {
                d->pos = value.toInt();
                Q_EMIT posChanged(d->pos);
                break;
            }
            case ModelStrategy::P_Value: {
                d->value = value.toDouble();
                Q_EMIT valueChanged(d->value);
                break;
            }
            default:
                Q_UNREACHABLE();
        }
    }

}

#include "moc_Tempo.cpp"
