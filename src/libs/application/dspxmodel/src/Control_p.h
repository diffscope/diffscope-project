#ifndef DIFFSCOPE_DSPX_MODEL_CONTROL_P_H
#define DIFFSCOPE_DSPX_MODEL_CONTROL_P_H

#include <dspxmodel/Control.h>

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

}

#endif //DIFFSCOPE_DSPX_MODEL_CONTROL_P_H
