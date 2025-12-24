#ifndef DIFFSCOPE_DSPX_MODEL_PARAMCURVESEQUENCE_P_H
#define DIFFSCOPE_DSPX_MODEL_PARAMCURVESEQUENCE_P_H

#include <dspxmodel/ParamCurveSequence.h>

#include <dspxmodel/private/PointSequenceData_p.h>
#include <dspxmodel/private/ParamCurve_p.h>

namespace dspx {

    class ParamCurveSequencePrivate : public PointSequenceData<ParamCurveSequence, ParamCurve, &ParamCurve::start, &ParamCurve::startChanged> {
        Q_DECLARE_PUBLIC(ParamCurveSequence)
    public:
        Param *param{};
    };

}

#endif //DIFFSCOPE_DSPX_MODEL_PARAMCURVESEQUENCE_P_H