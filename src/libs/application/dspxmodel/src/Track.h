#ifndef DIFFSCOPE_DSPX_MODEL_TRACK_H
#define DIFFSCOPE_DSPX_MODEL_TRACK_H

#include <qqmlintegration.h>

#include <dspxmodel/EntityObject.h>

namespace QDspx {
    struct Track;
}

namespace dspx {

    class TrackControl;
    class Workspace;

    class TrackPrivate;

    class DSPX_MODEL_EXPORT Track : public EntityObject {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_DECLARE_PRIVATE(Track)
        Q_PROPERTY(TrackControl *control READ control CONSTANT)
        Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
        Q_PROPERTY(Workspace *workspace READ workspace CONSTANT)

    public:
        ~Track() override;

        TrackControl *control() const;

        QString name() const;
        void setName(const QString &name);

        Workspace *workspace() const;

        QDspx::Track toQDspx() const;
        void fromQDspx(const QDspx::Track &track);

    Q_SIGNALS:
        void nameChanged(const QString &name);

    protected:
        void handleSetEntityProperty(int property, const QVariant &value) override;

    private:
        friend class ModelPrivate;
        explicit Track(Handle handle, Model *model);
        QScopedPointer<TrackPrivate> d_ptr;

    };

}

#endif //DIFFSCOPE_DSPX_MODEL_TRACK_H
