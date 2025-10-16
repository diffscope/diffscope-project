#include "TrackControl.h"

#include <opendspx/qdspxmodel.h>

#include <dspxmodel/private/Control_p.h>
#include <dspxmodel/private/Model_p.h>
#include <dspxmodel/ModelStrategy.h>

namespace dspx {

    class TrackControlPrivate : public ControlPrivate {
        Q_DECLARE_PUBLIC(TrackControl)
    public:
        bool solo;
    };

    TrackControl::TrackControl(Handle handle, Model *model) : Control(handle, model, *new TrackControlPrivate) {
        Q_D(TrackControl);
        d->solo = d->pModel->strategy->getEntityProperty(handle, ModelStrategy::P_ControlSolo).toBool();
    }

    TrackControl::~TrackControl() = default;

    bool TrackControl::solo() const {
        Q_D(const TrackControl);
        return d->solo;
    }

    void TrackControl::setSolo(bool solo) {
        Q_D(TrackControl);
        d->pModel->strategy->setEntityProperty(d->handle, ModelStrategy::P_ControlSolo, solo);
    }

    QDspx::TrackControl TrackControl::toQDspx() const {
        return {
            gain(),
            pan(),
            mute(),
            solo()
        };
    }
    void TrackControl::fromQDspx(const QDspx::TrackControl &trackControl) {
        setGain(trackControl.gain);
        setPan(trackControl.pan);
        setMute(trackControl.mute);
        setSolo(trackControl.solo);
    }

    bool TrackControl::handleProxySetEntityProperty(int property, const QVariant &value) {
        Q_D(TrackControl);
        switch (property) {
            case ModelStrategy::P_ControlSolo: {
                d->solo = value.toBool();
                Q_EMIT soloChanged(d->solo);
                return true;
            }
            default: {
                if (Control::handleProxySetEntityProperty(property, value))
                    return true;
                return false;
            }
        }
    }

}