#include "ParamCurveSequence.h"
#include "ParamCurveSequence_p.h"

#include <QJSEngine>
#include <QJSValue>

#include <opendspx/paramcurve.h>
#include <opendspx/paramcurveanchor.h>
#include <opendspx/paramcurvefree.h>

#include <dspxmodel/ModelStrategy.h>
#include <dspxmodel/ParamCurve.h>
#include <dspxmodel/Param.h>
#include <dspxmodel/ParamCurveAnchor.h>
#include <dspxmodel/ParamCurveFree.h>
#include <dspxmodel/private/Model_p.h>
#include <dspxmodel/private/PointSequenceContainer_p.h>
#include <dspxmodel/private/PointSequenceData_p.h>

namespace dspx {

    ParamCurveSequence::ParamCurveSequence(Param *param, Handle handle, Model *model) : EntityObject(handle, model), d_ptr(new ParamCurveSequencePrivate) {
        Q_D(ParamCurveSequence);
        Q_ASSERT(model->strategy()->getEntityType(handle) == ModelStrategy::ES_ParamCurves);
        d->q_ptr = this;
        d->pModel = ModelPrivate::get(model);
        d->param = param;

        d->init(model->strategy()->getEntitiesFromSequenceContainer(handle));
    }

    ParamCurveSequence::~ParamCurveSequence() = default;

    int ParamCurveSequence::size() const {
        Q_D(const ParamCurveSequence);
        return d->container.size();
    }

    ParamCurve *ParamCurveSequence::firstItem() const {
        Q_D(const ParamCurveSequence);
        return d->firstItem;
    }

    ParamCurve *ParamCurveSequence::lastItem() const {
        Q_D(const ParamCurveSequence);
        return d->lastItem;
    }

    ParamCurve *ParamCurveSequence::previousItem(ParamCurve *item) const {
        Q_D(const ParamCurveSequence);
        return d->container.previousItem(item);
    }

    ParamCurve *ParamCurveSequence::nextItem(ParamCurve *item) const {
        Q_D(const ParamCurveSequence);
        return d->container.nextItem(item);
    }

    QList<ParamCurve *> ParamCurveSequence::slice(int position, int length) const {
        Q_D(const ParamCurveSequence);
        return d->container.slice(position, length);
    }

    bool ParamCurveSequence::contains(ParamCurve *item) const {
        Q_D(const ParamCurveSequence);
        return d->container.contains(item);
    }

    bool ParamCurveSequence::insertItem(ParamCurve *item) {
        Q_D(ParamCurveSequence);
        return d->pModel->strategy->insertIntoSequenceContainer(handle(), item->handle());
    }

    bool ParamCurveSequence::removeItem(ParamCurve *item) {
        Q_D(ParamCurveSequence);
        return d->pModel->strategy->takeFromSequenceContainer(handle(), item->handle());
    }

    std::vector<opendspx::ParamCurveRef> ParamCurveSequence::toOpenDspx() const {
        Q_D(const ParamCurveSequence);
        std::vector<opendspx::ParamCurveRef> ret;
        ret.reserve(d->container.size());
        for (const auto &[_, item] : d->container.m_items) {
            ret.push_back(item->toOpenDspx());
        }
        return ret;
    }

    void ParamCurveSequence::fromOpenDspx(const std::vector<opendspx::ParamCurveRef> &curves) {
        while (size()) {
            removeItem(firstItem());
        }
        for (const auto &curve : curves) {
            ParamCurve *item = nullptr;
            switch (curve->type) {
                case opendspx::ParamCurve::Anchor:
                    item = model()->createParamCurveAnchor();
                    break;
                case opendspx::ParamCurve::Free:
                    item = model()->createParamCurveFree();
                    break;
                default:
                    Q_UNREACHABLE();
            }
            item->fromOpenDspx(curve);
            insertItem(item);
        }
    }

    void ParamCurveSequence::handleInsertIntoSequenceContainer(Handle entity) {
        Q_D(ParamCurveSequence);
        d->handleInsertIntoSequenceContainer(entity);
    }

    void ParamCurveSequence::handleTakeFromSequenceContainer(Handle takenEntity, Handle entity) {
        Q_D(ParamCurveSequence);
        d->handleTakeFromSequenceContainer(takenEntity, entity);
    }

    Param *ParamCurveSequence::param() const {
        Q_D(const ParamCurveSequence);
        return d->param;
    }

}

#include "moc_ParamCurveSequence.cpp"
