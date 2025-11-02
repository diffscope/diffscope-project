#include "ParamCurveFree.h"

#include <opendspx/paramcurvefree.h>

#include <dspxmodel/FreeValueDataArray.h>
#include <dspxmodel/ModelStrategy.h>
#include <dspxmodel/private/Model_p.h>

namespace dspx {

    class ParamCurveFreePrivate {
        Q_DECLARE_PUBLIC(ParamCurveFree)
    public:
        ParamCurveFree *q_ptr;
        ModelPrivate *pModel;
        FreeValueDataArray *values;
    };

    ParamCurveFree::ParamCurveFree(Handle handle, Model *model)
        : ParamCurve(Free, handle, model), d_ptr(new ParamCurveFreePrivate) {
        Q_D(ParamCurveFree);
        Q_ASSERT(model->strategy()->getEntityType(handle) == ModelStrategy::EI_ParamCurveFree);
        d->q_ptr = this;
        d->pModel = ModelPrivate::get(model);

        d->values = d->pModel->createObject<FreeValueDataArray>(d->pModel->strategy->getAssociatedSubEntity(handle, ModelStrategy::R_Children));
    }

    ParamCurveFree::~ParamCurveFree() = default;

    int ParamCurveFree::step() const {
        Q_D(const ParamCurveFree);
        return 5;
    }

    FreeValueDataArray *ParamCurveFree::values() const {
        Q_D(const ParamCurveFree);
        return d->values;
    }

    QDspx::ParamCurveFree ParamCurveFree::toQDspx() const {
        Q_D(const ParamCurveFree);
        return {
            start(),
            5,
            values()->toQDspx(),
        };
    }

    void ParamCurveFree::fromQDspx(const QDspx::ParamCurveFree &curve) {
        setStart(curve.start);
        values()->fromQDspx(curve.values);
    }

    void ParamCurveFree::handleSetEntityProperty(int property, const QVariant &value) {
        ParamCurve::handleSetEntityProperty(property, value);
    }

}

#include "moc_ParamCurveFree.cpp"
