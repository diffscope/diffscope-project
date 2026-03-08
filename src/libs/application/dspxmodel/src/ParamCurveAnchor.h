#ifndef DIFFSCOPE_DSPX_MODEL_PARAMCURVEANCHOR_H
#define DIFFSCOPE_DSPX_MODEL_PARAMCURVEANCHOR_H

#include <qqmlintegration.h>

#include <dspxmodel/ParamCurve.h>

namespace QDspx {
    struct ParamCurveAnchor;
}

namespace dspx {

    class AnchorNodeSequence;
    class ParamCurveAnchorPrivate;

    class DSPX_MODEL_EXPORT ParamCurveAnchor : public ParamCurve {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_DECLARE_PRIVATE(ParamCurveAnchor)
        Q_PROPERTY(AnchorNodeSequence *nodes READ nodes CONSTANT)

    public:
        ~ParamCurveAnchor() override;

        AnchorNodeSequence *nodes() const;

        QDspx::ParamCurveAnchor toQDspx() const;
        void fromQDspx(const QDspx::ParamCurveAnchor &curve);

    protected:
        void handleSetEntityProperty(int property, const QVariant &value) override;

    private:
        friend class ModelPrivate;
        explicit ParamCurveAnchor(Handle handle, Model *model);
        QScopedPointer<ParamCurveAnchorPrivate> d_ptr;
    };

}

#endif //DIFFSCOPE_DSPX_MODEL_PARAMCURVEANCHOR_H
