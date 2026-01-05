#ifndef DIFFSCOPE_DSPX_MODEL_MASTER_H
#define DIFFSCOPE_DSPX_MODEL_MASTER_H

#include <QObject>
#include <qqmlintegration.h>

#include <dspxmodel/DspxModelGlobal.h>

namespace QDspx {
    struct Master;
}

namespace dspx {

    class BusControl;

    class Model;

    class ModelPrivate;

    class MasterPrivate;

    class DSPX_MODEL_EXPORT Master : public QObject {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_DECLARE_PRIVATE(Master)
        Q_PROPERTY(BusControl *control READ control CONSTANT)
        Q_PROPERTY(bool multiChannelOutput READ multiChannelOutput WRITE setMultiChannelOutput NOTIFY multiChannelOutputChanged)
    public:
        ~Master() override;

        BusControl *control() const;

        bool multiChannelOutput() const;
        void setMultiChannelOutput(bool multiChannelOutput);

        QDspx::Master toQDspx() const;
        void fromQDspx(const QDspx::Master &master);

    Q_SIGNALS:
        void multiChannelOutputChanged(bool multiChannelOutput);

    private:
        friend class ModelPrivate;
        explicit Master(Model *model);
        void handleProxySetEntityProperty(int property, const QVariant &value);
        QScopedPointer<MasterPrivate> d_ptr;
    };

}

#endif //DIFFSCOPE_DSPX_MODEL_MASTER_H
