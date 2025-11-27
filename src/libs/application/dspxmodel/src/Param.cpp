#include "Param.h"
#include "Param_p.h"

#include <opendspx/param.h>

#include <dspxmodel/ModelStrategy.h>
#include <dspxmodel/ParamCurveSequence.h>
#include <dspxmodel/ParamMap.h>
#include <dspxmodel/private/Model_p.h>

namespace dspx {

    void ParamPrivate::setParamMap(Param *item, ParamMap *paramMap) {
        auto d = item->d_func();
        if (d->paramMap != paramMap) {
            d->paramMap = paramMap;
            Q_EMIT item->paramMapChanged();
        }
    }

    Param::Param(Handle handle, Model *model) : EntityObject(handle, model), d_ptr(new ParamPrivate) {
        Q_D(Param);
        Q_ASSERT(model->strategy()->getEntityType(handle) == ModelStrategy::EI_Param);
        d->q_ptr = this;
        d->pModel = ModelPrivate::get(model);
        d->paramMap = nullptr;

        d->original = d->pModel->createObject<ParamCurveSequence>(this, d->pModel->strategy->getAssociatedSubEntity(handle, ModelStrategy::R_ParamCurvesOriginal));
        d->transform = d->pModel->createObject<ParamCurveSequence>(this, d->pModel->strategy->getAssociatedSubEntity(handle, ModelStrategy::R_ParamCurvesTransform));
        d->edited = d->pModel->createObject<ParamCurveSequence>(this, d->pModel->strategy->getAssociatedSubEntity(handle, ModelStrategy::R_ParamCurvesEdited));
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
        return {
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

    ParamMap *Param::paramMap() const {
        Q_D(const Param);
        return d->paramMap;
    }

}

#include "moc_Param.cpp"
