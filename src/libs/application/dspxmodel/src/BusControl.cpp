#include "BusControl.h"

#include <opendspx/qdspxmodel.h>

namespace dspx {

    BusControl::BusControl(Handle handle, Model *model) : Control(handle, model) {
    }

    BusControl::~BusControl() = default;

    QDspx::Control BusControl::toQDspx() const {
        return {
            gain(),
            pan(),
            mute()
        };
    }

    void BusControl::fromQDspx(const QDspx::Control &control) {
        setGain(control.gain);
        setPan(control.pan);
        setMute(control.mute);
    }

    bool BusControl::handleProxySetEntityProperty(int property, const QVariant &value) {
        if (Control::handleProxySetEntityProperty(property, value))
            return true;
        return false;
    }

}