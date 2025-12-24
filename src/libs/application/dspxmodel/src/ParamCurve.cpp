#include "ParamCurve.h"
#include "ParamCurve_p.h"

#include <opendspx/paramcurve.h>
#include <opendspx/paramcurveanchor.h>
#include <opendspx/paramcurvefree.h>

#include <dspxmodel/ModelStrategy.h>
#include <dspxmodel/ParamCurveAnchor.h>
#include <dspxmodel/ParamCurveFree.h>
#include <dspxmodel/Param.h>
#include <dspxmodel/ParamCurveSequence.h>
#include <dspxmodel/private/Model_p.h>

namespace dspx {

    void ParamCurvePrivate::setParamCurveSequence(ParamCurve *paramCurve, ParamCurveSequence *paramCurveSequence) {
        auto d = paramCurve->d_func();
        if (d->paramCurveSequence == paramCurveSequence)
            return;
        d->paramCurveSequence = paramCurveSequence;
        Q_EMIT paramCurve->paramCurveSequenceChanged();
    }

    ParamCurve::ParamCurve(CurveType type, Handle handle, Model *model)
        : EntityObject(handle, model), d_ptr(new ParamCurvePrivate) {
        Q_D(ParamCurve);
        d->q_ptr = this;
        d->pModel = ModelPrivate::get(model);
        d->type = type;
        d->paramCurveSequence = nullptr;
    }

    ParamCurve::~ParamCurve() = default;

    int ParamCurve::start() const {
        Q_D(const ParamCurve);
        return d->pModel->strategy->getEntityProperty(handle(), ModelStrategy::P_Position).toInt();
    }

    void ParamCurve::setStart(int start) {
        Q_D(ParamCurve);
        d->pModel->strategy->setEntityProperty(handle(), ModelStrategy::P_Position, start);
    }

    ParamCurve::CurveType ParamCurve::type() const {
        Q_D(const ParamCurve);
        return d->type;
    }

    ParamCurveSequence *ParamCurve::paramCurveSequence() const {
        Q_D(const ParamCurve);
        return d->paramCurveSequence;
    }

    QDspx::ParamCurveRef ParamCurve::toQDspx() const {
        Q_D(const ParamCurve);
        switch (d->type) {
            case Anchor:
                return QSharedPointer<QDspx::ParamCurveAnchor>::create(static_cast<const ParamCurveAnchor *>(this)->toQDspx());
            case Free:
                return QSharedPointer<QDspx::ParamCurveFree>::create(static_cast<const ParamCurveFree *>(this)->toQDspx());
            default:
                Q_UNREACHABLE();
        }
    }

    void ParamCurve::fromQDspx(const QDspx::ParamCurveRef &curve) {
        switch (curve->type) {
            case QDspx::ParamCurve::Anchor:
                static_cast<ParamCurveAnchor *>(this)->fromQDspx(*curve.staticCast<QDspx::ParamCurveAnchor>());
                break;
            case QDspx::ParamCurve::Free:
                static_cast<ParamCurveFree *>(this)->fromQDspx(*curve.staticCast<QDspx::ParamCurveFree>());
                break;
            default:
                Q_UNREACHABLE();
        }
    }

    void ParamCurve::handleSetEntityProperty(int property, const QVariant &value) {
        Q_D(ParamCurve);
        switch (property) {
            case ModelStrategy::P_Position: {
                d->start = value.toInt();
                Q_EMIT startChanged(d->start);
                break;
            }
            default:
                Q_UNREACHABLE();
        }
    }

}

#include "moc_ParamCurve.cpp"
