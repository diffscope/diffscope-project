#ifndef DIFFSCOPE_DSPX_MODEL_PARAMCURVEFREE_H
#define DIFFSCOPE_DSPX_MODEL_PARAMCURVEFREE_H

#include <qqmlintegration.h>

#include <dspxmodel/ParamCurve.h>

namespace QDspx {
    struct ParamCurveFree;
}

namespace dspx {

    class FreeValueDataArray;
    class ParamCurveFreePrivate;

    class DSPX_MODEL_EXPORT ParamCurveFree : public ParamCurve {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_DECLARE_PRIVATE(ParamCurveFree)
        Q_PROPERTY(int step READ step CONSTANT)
        Q_PROPERTY(FreeValueDataArray *values READ values CONSTANT)

    public:
        ~ParamCurveFree() override;

        int step() const;
        FreeValueDataArray *values() const;

        QDspx::ParamCurveFree toQDspx() const;
        void fromQDspx(const QDspx::ParamCurveFree &curve);

    protected:
        void handleSetEntityProperty(int property, const QVariant &value) override;

    private:
        friend class ModelPrivate;
        explicit ParamCurveFree(Handle handle, Model *model);
        QScopedPointer<ParamCurveFreePrivate> d_ptr;

    };

}

#endif //DIFFSCOPE_DSPX_MODEL_PARAMCURVEFREE_H