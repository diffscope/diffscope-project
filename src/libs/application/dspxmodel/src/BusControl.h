#ifndef DIFFSCOPE_DSPX_MODEL_BUSCONTROL_H
#define DIFFSCOPE_DSPX_MODEL_BUSCONTROL_H

#include <dspxmodel/Control.h>

namespace QDspx {
    struct Control;
}

namespace dspx {

    class BusControl : public Control {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")

    public:
        ~BusControl() override;

        QDspx::Control toQDspx() const;
        void fromQDspx(const QDspx::Control &control);

    private:
        friend class ModelPrivate;
        explicit BusControl(Handle handle, Model *model);
        bool handleProxySetEntityProperty(int property, const QVariant &value);

    };

}

#endif //DIFFSCOPE_DSPX_MODEL_BUSCONTROL_H
