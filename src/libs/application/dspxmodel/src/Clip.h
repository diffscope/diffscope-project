#ifndef DIFFSCOPE_DSPX_MODEL_CLIP_H
#define DIFFSCOPE_DSPX_MODEL_CLIP_H

#include <qqmlintegration.h>

#include <dspxmodel/EntityObject.h>

namespace QDspx {
    struct Clip;
    using ClipRef = QSharedPointer<Clip>;
}

namespace dspx {

    class BusControl;
    class ClipTime;
    class Workspace;
    class ClipSequence;

    class ClipPrivate;

    class DSPX_MODEL_EXPORT Clip : public EntityObject {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_DECLARE_PRIVATE(Clip)
        Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
        Q_PROPERTY(BusControl *control READ control CONSTANT)
        Q_PROPERTY(ClipTime *time READ time CONSTANT)
        Q_PROPERTY(ClipType type READ type CONSTANT)
        Q_PROPERTY(Workspace *workspace READ workspace CONSTANT)
        Q_PROPERTY(ClipSequence *clipSequence READ clipSequence NOTIFY clipSequenceChanged)
        Q_PROPERTY(Clip *previousItem READ previousItem NOTIFY previousItemChanged)
        Q_PROPERTY(Clip *nextItem READ nextItem NOTIFY nextItemChanged)
        Q_PROPERTY(bool overlapped READ isOverlapped NOTIFY overlappedChanged)
    public:
        enum ClipType {
            Audio,
            Singing,
        };
        Q_ENUM(ClipType)

        ~Clip() override;

        QString name() const;
        void setName(const QString &name);

        BusControl *control() const;

        ClipTime *time() const;

        ClipType type() const;

        Workspace *workspace() const;

        QDspx::ClipRef toQDspx() const;
        void fromQDspx(const QDspx::ClipRef &clip);

        ClipSequence *clipSequence() const;

        Clip *previousItem() const;
        Clip *nextItem() const;

        int position() const;

        int length() const;

        bool isOverlapped() const;

    Q_SIGNALS:
        void nameChanged(const QString &name);
        void clipSequenceChanged();
        void previousItemChanged();
        void nextItemChanged();
        void positionChanged(int position);
        void lengthChanged(int length);
        void overlappedChanged(bool overlapped);

    protected:
        explicit Clip(ClipType type, Handle handle, Model *model);
        void handleSetEntityProperty(int property, const QVariant &value) override;

    private:
        QScopedPointer<ClipPrivate> d_ptr;
    };

} // dspx

#endif //DIFFSCOPE_DSPX_MODEL_CLIP_H
