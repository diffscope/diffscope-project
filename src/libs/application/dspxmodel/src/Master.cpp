#include "Master.h"

#include <QJSEngine>
#include <QVariant>

#include <opendspx/master.h>

#include <dspxmodel/BusControl.h>
#include <dspxmodel/ModelStrategy.h>
#include <dspxmodel/private/Model_p.h>

namespace dspx {

    class MasterPrivate {
        Q_DECLARE_PUBLIC(Master)
    public:
        Master *q_ptr;
        ModelPrivate *pModel;
        Handle handle;
        BusControl *control;
        bool multiChannelOutput{};
    };

    Master::Master(Model *model) : QObject(model), d_ptr(new MasterPrivate) {
        Q_D(Master);
        d->q_ptr = this;
        d->pModel = ModelPrivate::get(model);
        d->handle = model->handle();
        d->control = d->pModel->createObject<BusControl>(model->handle());
    }
    void Master::handleProxySetEntityProperty(int property, const QVariant &value) {
        Q_D(Master);
        switch (property) {
            case ModelStrategy::P_ControlGain:
            case ModelStrategy::P_ControlPan:
            case ModelStrategy::P_ControlMute: {
                ModelPrivate::proxySetEntityPropertyNotify(d->control, property, value);
                break;
            }
            case ModelStrategy::P_MultiChannelOutput: {
                d->multiChannelOutput = value.toBool();
                Q_EMIT multiChannelOutputChanged(d->multiChannelOutput);
                break;
            }
            default:
                Q_UNREACHABLE();
        }

    }
    Master::~Master() = default;

    BusControl *Master::control() const {
        Q_D(const Master);
        return d->control;
    }
    bool Master::multiChannelOutput() const {
        Q_D(const Master);
        return d->multiChannelOutput;
    }
    void Master::setMultiChannelOutput(bool multiChannelOutput) {
        Q_D(Master);
        d->pModel->strategy->setEntityProperty(d->handle, ModelStrategy::P_MultiChannelOutput, multiChannelOutput);
    }

    QDspx::Master Master::toQDspx() const {
        return {
            .control = control()->toQDspx(),
        };
    }

    void Master::fromQDspx(const QDspx::Master &master) {
        Q_D(Master);
        d->control->fromQDspx(master.control);
    }

}

#include "moc_Master.cpp"
