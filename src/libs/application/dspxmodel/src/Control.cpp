#include "Control.h"

#include <QVariant>
#include <QJSEngine>

#include <dspxmodel/private/Model_p.h>
#include <dspxmodel/ModelStrategy.h>

namespace dspx {

    class ControlPrivate {
        Q_DECLARE_PUBLIC(Control)
    public:
        Control *q_ptr;
        ModelPrivate *pModel;
        Handle handle;

        double gain;
        double pan;
        bool mute;

        void setPanUnchecked(double pan_);
        void setPan(double pan_);
    };

    void ControlPrivate::setPanUnchecked(double pan_) {
        Q_Q(Control);
        pModel->strategy->setEntityProperty(handle, ModelStrategy::P_ControlPan, pan_);
    }

    void ControlPrivate::setPan(double pan_) {
        Q_Q(Control);
        if (auto engine = qjsEngine(q); engine && (pan_ < -1 || pan_ > 1)) {
            engine->throwError(QJSValue::RangeError, QStringLiteral("Pan must be in range [-1.0, 1.0]"));
            return;
        }
        setPanUnchecked(pan_);
    }

    Control::Control(Handle handle, Model *model) : QObject(model), d_ptr(new ControlPrivate) {
        Q_D(Control);
        d->q_ptr = this;
        d->pModel = ModelPrivate::get(model);
        d->handle = handle;
        d->gain = d->pModel->strategy->getEntityProperty(handle, ModelStrategy::P_ControlGain).toDouble();
        d->pan = d->pModel->strategy->getEntityProperty(handle, ModelStrategy::P_ControlPan).toDouble();
        d->mute = d->pModel->strategy->getEntityProperty(handle, ModelStrategy::P_ControlMute).toBool();
    }

    Control::~Control() = default;

    double Control::gain() const {
        Q_D(const Control);
        return d->gain;
    }

    void Control::setGain(double gain) {
        Q_D(Control);
        d->pModel->strategy->setEntityProperty(d->handle, ModelStrategy::P_ControlGain, gain);
    }

    double Control::pan() const {
        Q_D(const Control);
        return d->pan;
    }

    void Control::setPan(double pan) {
        Q_D(Control);
        Q_ASSERT(pan >= -1.0 && pan <= 1.0);
        d->setPanUnchecked(pan);
    }

    bool Control::mute() const {
        Q_D(const Control);
        return d->mute;
    }

    void Control::setMute(bool mute) {
        Q_D(Control);
        d->pModel->strategy->setEntityProperty(d->handle, ModelStrategy::P_ControlMute, mute);
    }

    void Control::handleProxySetEntityProperty(int property, const QVariant &value) {
        Q_D(Control);
        switch (property) {
            case ModelStrategy::P_ControlGain: {
                d->gain = value.toDouble();
                Q_EMIT gainChanged(d->gain);
                break;
            }
            case ModelStrategy::P_ControlPan: {
                d->pan = value.toDouble();
                Q_EMIT panChanged(d->pan);
                break;
            }
            case ModelStrategy::P_ControlMute: {
                d->mute = value.toBool();
                Q_EMIT muteChanged(d->mute);
                break;
            }
            default:
                Q_UNREACHABLE();
        }
    }

    Handle Control::handle() const {
        Q_D(const Control);
        return d->handle;
    }

}

#include "moc_Control.cpp"