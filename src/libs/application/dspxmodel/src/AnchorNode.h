#ifndef DIFFSCOPE_DSPX_MODEL_ANCHORNODE_H
#define DIFFSCOPE_DSPX_MODEL_ANCHORNODE_H

#include <dspxmodel/EntityObject.h>
#include <qqmlintegration.h>

namespace QDspx {
    struct AnchorNode;
}

namespace dspx {

    class AnchorNodePrivate;

    class DSPX_MODEL_EXPORT AnchorNode : public EntityObject {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_DECLARE_PRIVATE(AnchorNode)
        Q_PRIVATE_PROPERTY(d_func(), InterpolationMode interp MEMBER interp WRITE setInterp NOTIFY interpChanged)
        Q_PRIVATE_PROPERTY(d_func(), int x MEMBER x WRITE setX NOTIFY xChanged)
        Q_PROPERTY(int y READ y WRITE setY NOTIFY yChanged)

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

    Q_SIGNALS:
        void interpChanged(InterpolationMode interp);
        void xChanged(int x);
        void yChanged(int y);

    protected:
        void handleSetEntityProperty(int property, const QVariant &value) override;

    private:
        friend class ModelPrivate;
        explicit AnchorNode(Handle handle, Model *model);
        QScopedPointer<AnchorNodePrivate> d_ptr;
    };

}

#endif //DIFFSCOPE_DSPX_MODEL_ANCHORNODE_H