#include "ParamCurveAnchor.h"

#include <opendspx/paramcurveanchor.h>

#include <dspxmodel/private/Model_p.h>
#include <dspxmodel/ModelStrategy.h>
#include <dspxmodel/AnchorNodeSequence.h>

namespace dspx {

    class ParamCurveAnchorPrivate {
        Q_DECLARE_PUBLIC(ParamCurveAnchor)
    public:
        ParamCurveAnchor *q_ptr;
        ModelPrivate *pModel;
        AnchorNodeSequence *nodes;
    };

    ParamCurveAnchor::ParamCurveAnchor(Handle handle, Model *model)
        : ParamCurve(Anchor, handle, model), d_ptr(new ParamCurveAnchorPrivate) {
        Q_D(ParamCurveAnchor);
        Q_ASSERT(model->strategy()->getEntityType(handle) == ModelStrategy::EI_ParamCurveAnchor);
        d->q_ptr = this;
        d->pModel = ModelPrivate::get(model);

        d->nodes = d->pModel->createObject<AnchorNodeSequence>(d->pModel->strategy->getAssociatedSubEntity(handle, ModelStrategy::R_Children));
    }

    ParamCurveAnchor::~ParamCurveAnchor() = default;

    AnchorNodeSequence *ParamCurveAnchor::nodes() const {
        Q_D(const ParamCurveAnchor);
        return d->nodes;
    }

    QDspx::ParamCurveAnchor ParamCurveAnchor::toQDspx() const {
        return {
            start(),
            nodes()->toQDspx(),
        };
    }

    void ParamCurveAnchor::fromQDspx(const QDspx::ParamCurveAnchor &curve) {
        setStart(curve.start);
        nodes()->fromQDspx(curve.nodes);
    }

    void ParamCurveAnchor::handleSetEntityProperty(int property, const QVariant &value) {
        ParamCurve::handleSetEntityProperty(property, value);
    }

}

#include "moc_ParamCurveAnchor.cpp"