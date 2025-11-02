#ifndef DIFFSCOPE_DSPX_MODEL_BUSCONTROL_H
#define DIFFSCOPE_DSPX_MODEL_BUSCONTROL_H

#include <dspxmodel/Control.h>

namespace QDspx {
    struct BusControl;
}

namespace dspx {

    class BusControl : public Control {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")

    public:
        ~BusControl() override;

        QDspx::BusControl toQDspx() const;
        void fromQDspx(const QDspx::BusControl &control);

    private:
        friend class ModelPrivate;
        explicit BusControl(Handle handle, Model *model);
        void handleProxySetEntityProperty(int property, const QVariant &value);
    };

}

#endif //DIFFSCOPE_DSPX_MODEL_BUSCONTROL_H
