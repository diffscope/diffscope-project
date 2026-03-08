#include "TrackControl.h"

#include <opendspx/trackcontrol.h>

#include <dspxmodel/ModelStrategy.h>
#include <dspxmodel/private/Model_p.h>

namespace dspx {

    class TrackControlPrivate {
        Q_DECLARE_PUBLIC(TrackControl)
    public:
        TrackControl *q_ptr;
        ModelPrivate *pModel;
        bool solo;
        bool record;
    };

    TrackControl::TrackControl(Handle handle, Model *model) : Control(handle, model), d_ptr(new TrackControlPrivate) {
        Q_D(TrackControl);
        d->q_ptr = this;
        d->pModel = ModelPrivate::get(model);
        d->solo = d->pModel->strategy->getEntityProperty(handle, ModelStrategy::P_ControlSolo).toBool();
        d->record = d->pModel->strategy->getEntityProperty(handle, ModelStrategy::P_ControlRecord).toBool();
    }

    TrackControl::~TrackControl() = default;

    bool TrackControl::solo() const {
        Q_D(const TrackControl);
        return d->solo;
    }

    void TrackControl::setSolo(bool solo) {
        Q_D(TrackControl);
        d->pModel->strategy->setEntityProperty(handle(), ModelStrategy::P_ControlSolo, solo);
    }

    bool TrackControl::record() const {
        Q_D(const TrackControl);
        return d->record;
    }

    void TrackControl::setRecord(bool record) {
        Q_D(TrackControl);
        d->pModel->strategy->setEntityProperty(handle(), ModelStrategy::P_ControlRecord, record);
    }

    QDspx::TrackControl TrackControl::toQDspx() const {
        return {
            .gain = gain(),
            .pan = pan(),
            .mute = mute(),
            .solo = solo()
        };
    }
    void TrackControl::fromQDspx(const QDspx::TrackControl &trackControl) {
        setGain(trackControl.gain);
        setPan(trackControl.pan);
        setMute(trackControl.mute);
        setSolo(trackControl.solo);
    }

    void TrackControl::handleProxySetEntityProperty(int property, const QVariant &value) {
        Q_D(TrackControl);
        switch (property) {
            case ModelStrategy::P_ControlSolo: {
                d->solo = value.toBool();
                Q_EMIT soloChanged(d->solo);
                break;
            }
            case ModelStrategy::P_ControlRecord: {
                d->record = value.toBool();
                Q_EMIT recordChanged(d->record);
                break;
            }
            default: {
                Control::handleProxySetEntityProperty(property, value);
            }
        }
    }

}
