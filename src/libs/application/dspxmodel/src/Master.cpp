#include "Master.h"

#include <QVariant>

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
    };

    Master::Master(Model *model) : QObject(model), d_ptr(new MasterPrivate) {
        Q_D(Master);
        d->q_ptr = this;
        d->pModel = ModelPrivate::get(model);
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
        return d->pModel->pan;
    }
    void Master::setPan(double pan) {
        Q_D(Master);
        d->pModel->strategy->setEntityProperty(d->handle, ModelStrategy::P_ControlPan, pan);
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
            .control = {
                .gain = gain(),
                .pan = pan(),
                .mute = mute(),
            }
        };
    }

    void Master::fromQDspx(const QDspx::Master &master) {
        setGain(master.control.gain);
        setPan(master.control.pan);
        setMute(master.control.mute);
    }

}