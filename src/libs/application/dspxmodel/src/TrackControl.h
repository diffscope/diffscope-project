#ifndef DIFFSCOPE_DSPX_MODEL_TRACKCONTROL_H
#define DIFFSCOPE_DSPX_MODEL_TRACKCONTROL_H

#include <dspxmodel/Control.h>

namespace QDspx {
    struct TrackControl;
}

namespace dspx {

    class TrackControlPrivate;

    class TrackControl : public Control {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_DECLARE_PRIVATE(TrackControl)
        Q_PROPERTY(bool solo READ solo WRITE setSolo NOTIFY soloChanged)

    public:
        ~TrackControl() override;

        bool solo() const;
        void setSolo(bool solo);

        QDspx::TrackControl toQDspx() const;
        void fromQDspx(const QDspx::TrackControl &trackControl);

    Q_SIGNALS:
        void soloChanged(bool solo);

    private:
        friend class ModelPrivate;
        explicit TrackControl(Handle handle, Model *model);
        bool handleProxySetEntityProperty(int property, const QVariant &value);
        QScopedPointer<TrackControlPrivate> d_ptr;

    };

}

#endif //DIFFSCOPE_DSPX_MODEL_TRACKCONTROL_H
