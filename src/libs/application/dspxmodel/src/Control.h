#ifndef DIFFSCOPE_DSPX_MODEL_CONTROL_H
#define DIFFSCOPE_DSPX_MODEL_CONTROL_H

#include <QObject>
#include <qqmlintegration.h>

#include <dspxmodel/Handle.h>

namespace dspx {

    class Model;
    class ModelPrivate;

    class ControlPrivate;

    class Control : public QObject {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_DECLARE_PRIVATE(Control)
        Q_PROPERTY(double gain READ gain WRITE setGain NOTIFY gainChanged)
        Q_PRIVATE_PROPERTY(d_func(), double pan MEMBER pan WRITE setPan NOTIFY panChanged)
        Q_PROPERTY(bool mute READ mute WRITE setMute NOTIFY muteChanged)

    public:
        ~Control() override;

        double gain() const;
        void setGain(double gain);

        double pan() const;
        void setPan(double pan);

        bool mute() const;
        void setMute(bool mute);

    Q_SIGNALS:
        void gainChanged(double gain);
        void panChanged(double pan);
        void muteChanged(bool mute);

    protected:
        bool handleProxySetEntityProperty(int property, const QVariant &value);
        explicit Control(Handle handle, Model *model, ControlPrivate &d);
        QScopedPointer<ControlPrivate> d_ptr;

    };

}

#endif //DIFFSCOPE_DSPX_MODEL_CONTROL_H
