#ifndef DIFFSCOPE_DSPX_MODEL_MASTER_H
#define DIFFSCOPE_DSPX_MODEL_MASTER_H

#include <QObject>
#include <qqmlintegration.h>

namespace QDspx {
    struct Master;
}

namespace dspx {

    class Model;

    class ModelPrivate;

    class MasterPrivate;

    class Master : public QObject {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_DECLARE_PRIVATE(Master)
        Q_PROPERTY(double gain READ gain WRITE setGain NOTIFY gainChanged)
        Q_PROPERTY(double pan READ pan WRITE setPan NOTIFY panChanged)
        Q_PROPERTY(bool mute READ mute WRITE setMute NOTIFY muteChanged)

    public:
        ~Master() override;

        double gain() const;
        void setGain(double gain);

        double pan() const;
        void setPan(double pan);

        bool mute() const;
        void setMute(bool mute);

        QDspx::Master toQDspx() const;
        void fromQDspx(const QDspx::Master &master);

    Q_SIGNALS:
        void gainChanged(double gain);
        void panChanged(double pan);
        void muteChanged(bool mute);

    private:
        friend class ModelPrivate;
        explicit Master(Model *model);

        QScopedPointer<MasterPrivate> d_ptr;

    };

}

#endif //DIFFSCOPE_DSPX_MODEL_MASTER_H