#include "BusControl.h"

#include <opendspx/buscontrol.h>

namespace dspx {

    BusControl::BusControl(Handle handle, Model *model) : Control(handle, model) {
    }

    BusControl::~BusControl() = default;

    opendspx::BusControl BusControl::toOpenDspx() const {
        return {
            .gain = gain(),
            .pan = pan(),
            .mute = mute()
        };
    }

    void BusControl::fromOpenDspx(const opendspx::BusControl &control) {
        setGain(control.gain);
        setPan(control.pan);
        setMute(control.mute);
    }

    void BusControl::handleProxySetEntityProperty(int property, const QVariant &value) {
        Control::handleProxySetEntityProperty(property, value);
    }

}
