#ifndef DIFFSCOPE_DSPX_MODEL_PARAMCURVE_P_H
#define DIFFSCOPE_DSPX_MODEL_PARAMCURVE_P_H

#include <dspxmodel/ParamCurve.h>

namespace dspx {

    class ParamCurveSequence;
    class ModelPrivate;

    class ParamCurvePrivate {
        Q_DECLARE_PUBLIC(ParamCurve)
    public:
        ParamCurve *q_ptr;
        ModelPrivate *pModel;
        ParamCurve::CurveType type;
        int start{};
        ParamCurveSequence *paramCurveSequence{};

        static void setParamCurveSequence(ParamCurve *item, ParamCurveSequence *paramCurveSequence);
    };

}

#endif //DIFFSCOPE_DSPX_MODEL_PARAMCURVE_P_H