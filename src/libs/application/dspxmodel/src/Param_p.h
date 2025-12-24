#ifndef DIFFSCOPE_DSPX_MODEL_PARAM_P_H
#define DIFFSCOPE_DSPX_MODEL_PARAM_P_H

#include <dspxmodel/Param.h>

namespace dspx {

    class ParamMap;
    class ModelPrivate;

    class ParamPrivate {
        Q_DECLARE_PUBLIC(Param)
    public:
        Param *q_ptr;
        ModelPrivate *pModel;
        ParamCurveSequence *original;
        ParamCurveSequence *transform;
        ParamCurveSequence *edited;
        ParamMap *paramMap;

        static void setParamMap(Param *item, ParamMap *paramMap);
    };

}

#endif //DIFFSCOPE_DSPX_MODEL_PARAM_P_H