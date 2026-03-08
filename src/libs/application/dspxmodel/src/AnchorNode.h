#ifndef DIFFSCOPE_DSPX_MODEL_ANCHORNODE_H
#define DIFFSCOPE_DSPX_MODEL_ANCHORNODE_H

#include <qqmlintegration.h>

#include <dspxmodel/EntityObject.h>

namespace QDspx {
    struct AnchorNode;
}

namespace dspx {

    class AnchorNodeSequence;

    class AnchorNodeSequencePrivate;

    class AnchorNodePrivate;

    class DSPX_MODEL_EXPORT AnchorNode : public EntityObject {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_DECLARE_PRIVATE(AnchorNode)
        Q_PROPERTY(InterpolationMode interp READ interp WRITE setInterp NOTIFY interpChanged)
        Q_PROPERTY(int x READ x WRITE setX NOTIFY xChanged)
        Q_PROPERTY(int y READ y WRITE setY NOTIFY yChanged)
        Q_PROPERTY(AnchorNodeSequence *anchorNodeSequence READ anchorNodeSequence NOTIFY anchorNodeSequenceChanged)
        Q_PROPERTY(AnchorNode *previousItem READ previousItem NOTIFY previousItemChanged)
        Q_PROPERTY(AnchorNode *nextItem READ nextItem NOTIFY nextItemChanged)

    public:
        enum InterpolationMode {
            None = 0,
            Linear,
            Hermite
        };
        Q_ENUM(InterpolationMode)

        ~AnchorNode() override;

        InterpolationMode interp() const;
        void setInterp(InterpolationMode interp);

        int x() const;
        void setX(int x);

        int y() const;
        void setY(int y);

        QDspx::AnchorNode toQDspx() const;
        void fromQDspx(const QDspx::AnchorNode &node);

        AnchorNodeSequence *anchorNodeSequence() const;
        AnchorNode *previousItem() const;
        AnchorNode *nextItem() const;

    Q_SIGNALS:
        void interpChanged(InterpolationMode interp);
        void xChanged(int x);
        void yChanged(int y);
        void anchorNodeSequenceChanged();
        void previousItemChanged();
        void nextItemChanged();

    protected:
        void handleSetEntityProperty(int property, const QVariant &value) override;

    private:
        friend class ModelPrivate;
        explicit AnchorNode(Handle handle, Model *model);
        QScopedPointer<AnchorNodePrivate> d_ptr;
    };

}

#endif //DIFFSCOPE_DSPX_MODEL_ANCHORNODE_H
