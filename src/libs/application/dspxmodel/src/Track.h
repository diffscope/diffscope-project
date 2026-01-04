#ifndef DIFFSCOPE_DSPX_MODEL_TRACK_H
#define DIFFSCOPE_DSPX_MODEL_TRACK_H

#include <qqmlintegration.h>

#include <dspxmodel/EntityObject.h>

namespace QDspx {
    struct Track;
}

namespace dspx {

    class ClipSequence;
    class TrackControl;
    class Workspace;

    class TrackList;

    class TrackPrivate;

    class DSPX_MODEL_EXPORT Track : public EntityObject {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_DECLARE_PRIVATE(Track)
        Q_PROPERTY(ClipSequence *clips READ clips CONSTANT)
        Q_PROPERTY(int colorId READ colorId WRITE setColorId NOTIFY colorIdChanged)
        Q_PROPERTY(double height READ height WRITE setHeight NOTIFY heightChanged)
        Q_PROPERTY(TrackControl *control READ control CONSTANT)
        Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
        Q_PROPERTY(Workspace *workspace READ workspace CONSTANT)
        Q_PROPERTY(TrackList *trackList READ trackList CONSTANT)

    public:
        ~Track() override;

        ClipSequence *clips() const;

        int colorId() const;
        void setColorId(int colorId);

        double height() const;
        void setHeight(double height);

        TrackControl *control() const;

        QString name() const;
        void setName(const QString &name);

        Workspace *workspace() const;

        QDspx::Track toQDspx() const;
        void fromQDspx(const QDspx::Track &track);

        TrackList *trackList() const;

    Q_SIGNALS:
        void nameChanged(const QString &name);
        void colorIdChanged(int colorId);
        void heightChanged(double height);
        void trackListChanged();

    protected:
        void handleSetEntityProperty(int property, const QVariant &value) override;

    private:
        friend class ModelPrivate;
        explicit Track(Handle handle, Model *model);
        QScopedPointer<TrackPrivate> d_ptr;
    };

}

#endif //DIFFSCOPE_DSPX_MODEL_TRACK_H
