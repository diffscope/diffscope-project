#include "Tempo.h"

#include <QVariant>
#include <QJSEngine>

#include <opendspx/qdspxmodel.h>

#include <dspxmodel/Model.h>
#include <dspxmodel/ModelStrategy.h>

namespace dspx {

    class TempoPrivate {
        Q_DECLARE_PUBLIC(Tempo)
    public:
        Tempo *q_ptr;
        int pos;
        double value;

        void setPosUnchecked(int pos_);
        void setPos(int pos_);

        void setValueUnchecked(double value_);
        void setValue(double value_);

    };

    void TempoPrivate::setPosUnchecked(int pos_) {
        Q_Q(Tempo);
        q->model()->strategy()->setEntityProperty(q->handle(), ModelStrategy::P_Position, pos_);
    }

    void TempoPrivate::setPos(int pos_) {
        Q_Q(Tempo);
        if (auto engine = qjsEngine(q); engine && pos_ < 0) {
            engine->throwError(QJSValue::RangeError, QStringLiteral("Pos must be greater or equal to 0"));
            return;
        }
        setPosUnchecked(pos_);
    }

    void TempoPrivate::setValueUnchecked(double value_) {
        Q_Q(Tempo);
        q->model()->strategy()->setEntityProperty(q->handle(), ModelStrategy::P_Value, value_);
    }

    void TempoPrivate::setValue(double value_) {
        Q_Q(Tempo);
        if (auto engine = qjsEngine(q); engine && (value_ < 10.0 || value_ > 1000.0)) {
            engine->throwError(QJSValue::RangeError, QStringLiteral("Value must be in range [10.0, 1000.0]"));
            return;
        }
        setValueUnchecked(value_);
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
        d->setPosUnchecked(pos);
    }

    double Tempo::value() const {
        Q_D(const Tempo);
        return d->value;
    }

    void Tempo::setValue(double value) {
        Q_D(Tempo);
        Q_ASSERT(value >= 10.0 && value <= 1000.0);
        d->setValueUnchecked(value);
    }

    QDspx::Tempo Tempo::toQDspx() const {
        return QDspx::Tempo {
            pos(),
            value(),
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