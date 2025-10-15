#include "Master.h"

#include <QVariant>
#include <QJSEngine>

#include <opendspx/qdspxmodel.h>

#include <dspxmodel/ModelStrategy.h>
#include <dspxmodel/private/Model_p.h>
#include <dspxmodel/BusControl.h>

namespace dspx {

    class MasterPrivate {
        Q_DECLARE_PUBLIC(Master)
    public:
        Master *q_ptr;
        ModelPrivate *pModel;
        BusControl *control;
    };

    Master::Master(Model *model) : QObject(model), d_ptr(new MasterPrivate) {
        Q_D(Master);
        d->q_ptr = this;
        d->pModel = ModelPrivate::get(model);
        d->control = d->pModel->createObject<BusControl>(model->handle());
    }
    bool Master::handleProxySetEntityProperty(int property, const QVariant &value) {
        Q_D(Master);
        return ModelPrivate::proxySetEntityPropertyNotify(d->control, property, value);
    }
    Master::~Master() = default;

    BusControl *Master::control() const {
        Q_D(const Master);
        return d->control;
    }

    QDspx::Master Master::toQDspx() const {
        return {
            control()->toQDspx(),
        };
    }

    void Master::fromQDspx(const QDspx::Master &master) {
        Q_D(Master);
        d->control->fromQDspx(master.control);
    }

}

#include "moc_Master.cpp"