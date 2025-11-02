#ifndef DIFFSCOPE_DSPX_MODEL_MASTER_H
#define DIFFSCOPE_DSPX_MODEL_MASTER_H

#include <QObject>
#include <qqmlintegration.h>

namespace QDspx {
    struct Master;
}

namespace dspx {

    class BusControl;

    class Model;

    class ModelPrivate;

    class MasterPrivate;

    class Master : public QObject {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_DECLARE_PRIVATE(Master)
        Q_PROPERTY(BusControl *control READ control CONSTANT)

    public:
        ~Master() override;

        BusControl *control() const;

        QDspx::Master toQDspx() const;
        void fromQDspx(const QDspx::Master &master);

    private:
        friend class ModelPrivate;
        explicit Master(Model *model);
        void handleProxySetEntityProperty(int property, const QVariant &value);
        QScopedPointer<MasterPrivate> d_ptr;

    };

}

#endif //DIFFSCOPE_DSPX_MODEL_MASTER_H