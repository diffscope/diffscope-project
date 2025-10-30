#include "Param.h"

#include <opendspx/param.h>

#include <dspxmodel/private/Model_p.h>
#include <dspxmodel/ModelStrategy.h>
#include <dspxmodel/ParamCurveSequence.h>

namespace dspx {

    class ParamPrivate {
        Q_DECLARE_PUBLIC(Param)
    public:
        Param *q_ptr;
        ModelPrivate *pModel;
        ParamCurveSequence *original;
        ParamCurveSequence *transform;
        ParamCurveSequence *edited;
    };

    Param::Param(Handle handle, Model *model) : EntityObject(handle, model), d_ptr(new ParamPrivate) {
        Q_D(Param);
        Q_ASSERT(model->strategy()->getEntityType(handle) == ModelStrategy::EI_Param);
        d->q_ptr = this;
        d->pModel = ModelPrivate::get(model);

        d->original = d->pModel->createObject<ParamCurveSequence>(d->pModel->strategy->getAssociatedSubEntity(handle, ModelStrategy::R_ParamCurvesOriginal));
        d->transform = d->pModel->createObject<ParamCurveSequence>(d->pModel->strategy->getAssociatedSubEntity(handle, ModelStrategy::R_ParamCurvesTransform));
        d->edited = d->pModel->createObject<ParamCurveSequence>(d->pModel->strategy->getAssociatedSubEntity(handle, ModelStrategy::R_ParamCurvesEdited));
    }

    Param::~Param() = default;

    ParamCurveSequence *Param::original() const {
        Q_D(const Param);
        return d->original;
    }

    ParamCurveSequence *Param::transform() const {
        Q_D(const Param);
        return d->transform;
    }

    ParamCurveSequence *Param::edited() const {
        Q_D(const Param);
        return d->edited;
    }

    QDspx::Param Param::toQDspx() const {
        Q_D(const Param);
        return QDspx::Param{
            .original = d->original->toQDspx(),
            .transform = d->transform->toQDspx(),
            .edited = d->edited->toQDspx(),
        };
    }

    void Param::fromQDspx(const QDspx::Param &param) {
        Q_D(Param);
        d->original->fromQDspx(param.original);
        d->transform->fromQDspx(param.transform);
        d->edited->fromQDspx(param.edited);
    }

}

#include "moc_Param.cpp"