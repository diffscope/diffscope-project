#include "Master.h"

#include <QVariant>
#include <QJSEngine>

#include <opendspx/qdspxmodel.h>

#include <dspxmodel/Model.h>
#include <dspxmodel/ModelStrategy.h>
#include <dspxmodel/private/Model_p.h>

namespace dspx {

    class MasterPrivate {
        Q_DECLARE_PUBLIC(Master)
    public:
        Master *q_ptr;
        ModelPrivate *pModel;
        Handle handle;

        double pan() const;
        void setPanUnchecked(double pan);
        void setPan(double pan);
    };

    double MasterPrivate::pan() const {
        return pModel->pan;
    }

    void MasterPrivate::setPanUnchecked(double pan) {
        Q_Q(Master);
        pModel->strategy->setEntityProperty(handle, ModelStrategy::P_ControlPan, pan);
    }

    void MasterPrivate::setPan(double pan) {
        Q_Q(Master);
        if (auto engine = qjsEngine(q); engine && (pan < -1 || pan > 1)) {
            engine->throwError(QJSValue::RangeError, QStringLiteral("Pan must be in range [-1.0, 1.0]"));
            return;
        }
        setPanUnchecked(pan);
    }

    Master::Master(Model *model) : QObject(model), d_ptr(new MasterPrivate) {
        Q_D(Master);
        d->q_ptr = this;
        d->pModel = ModelPrivate::get(model);
        d->handle = model->handle();
    }
    Master::~Master() = default;

    double Master::gain() const {
        Q_D(const Master);
        return d->pModel->gain;
    }
    void Master::setGain(double gain) {
        Q_D(Master);
        d->pModel->strategy->setEntityProperty(d->handle, ModelStrategy::P_ControlGain, gain);
    }
    double Master::pan() const {
        Q_D(const Master);
        return d->pan();
    }
    void Master::setPan(double pan) {
        Q_D(Master);
        Q_ASSERT(pan >= -1.0 && pan <= 1.0);
        d->setPanUnchecked(pan);
    }
    bool Master::mute() const {
        Q_D(const Master);
        return d->pModel->mute;
    }
    void Master::setMute(bool mute) {
        Q_D(Master);
        d->pModel->strategy->setEntityProperty(d->handle, ModelStrategy::P_ControlMute, mute);
    }

    QDspx::Master Master::toQDspx() const {
        return {
            {
                gain(),
                pan(),
                mute(),
            }
        };
    }

    void Master::fromQDspx(const QDspx::Master &master) {
        setGain(master.control.gain);
        setPan(master.control.pan);
        setMute(master.control.mute);
    }

}

#include "moc_Master.cpp"