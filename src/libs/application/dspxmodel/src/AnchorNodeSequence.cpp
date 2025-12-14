#include "AnchorNodeSequence.h"
#include "AnchorNodeSequence_p.h"

#include <QJSEngine>
#include <QJSValue>

#include <opendspx/anchornode.h>

#include <dspxmodel/AnchorNode.h>
#include <dspxmodel/ModelStrategy.h>
#include <dspxmodel/ParamCurveAnchor.h>
#include <dspxmodel/private/Model_p.h>
#include <dspxmodel/private/PointSequenceData_p.h>

namespace dspx {

    AnchorNodeSequence::AnchorNodeSequence(ParamCurveAnchor *paramCurveAnchor, Handle handle, Model *model) : EntityObject(handle, model), d_ptr(new AnchorNodeSequencePrivate) {
        Q_D(AnchorNodeSequence);
        Q_ASSERT(model->strategy()->getEntityType(handle) == ModelStrategy::ES_ParamCurveAnchorNodes);
        d->q_ptr = this;
        d->pModel = ModelPrivate::get(model);
        d->paramCurveAnchor = paramCurveAnchor;

        d->init(model->strategy()->getEntitiesFromSequenceContainer(handle));

        connect(this, &AnchorNodeSequence::itemInserted, this, [=](AnchorNode *item) {
            AnchorNodePrivate::setAnchorNodeSequence(item, this);
        });
        connect(this, &AnchorNodeSequence::itemRemoved, this, [=](AnchorNode *item) {
            AnchorNodePrivate::setAnchorNodeSequence(item, nullptr);
        });
    }

    AnchorNodeSequence::~AnchorNodeSequence() = default;

    int AnchorNodeSequence::size() const {
        Q_D(const AnchorNodeSequence);
        return d->container.size();
    }

    AnchorNode *AnchorNodeSequence::firstItem() const {
        Q_D(const AnchorNodeSequence);
        return d->firstItem;
    }

    AnchorNode *AnchorNodeSequence::lastItem() const {
        Q_D(const AnchorNodeSequence);
        return d->lastItem;
    }

    AnchorNode *AnchorNodeSequence::previousItem(AnchorNode *item) const {
        Q_D(const AnchorNodeSequence);
        return d->container.previousItem(item);
    }

    AnchorNode *AnchorNodeSequence::nextItem(AnchorNode *item) const {
        Q_D(const AnchorNodeSequence);
        return d->container.nextItem(item);
    }

    QList<AnchorNode *> AnchorNodeSequence::slice(int position, int length) const {
        Q_D(const AnchorNodeSequence);
        return d->container.slice(position, length);
    }

    bool AnchorNodeSequence::contains(AnchorNode *item) const {
        Q_D(const AnchorNodeSequence);
        return d->container.contains(item);
    }

    bool AnchorNodeSequence::insertItem(AnchorNode *item) {
        Q_D(AnchorNodeSequence);
        return d->pModel->strategy->insertIntoSequenceContainer(handle(), item->handle());
    }

    bool AnchorNodeSequence::removeItem(AnchorNode *item) {
        Q_D(AnchorNodeSequence);
        return d->pModel->strategy->takeFromSequenceContainer(handle(), item->handle());
    }

    QList<QDspx::AnchorNode> AnchorNodeSequence::toQDspx() const {
        Q_D(const AnchorNodeSequence);
        QList<QDspx::AnchorNode> ret;
        ret.reserve(d->container.size());
        for (const auto &[_, item] : d->container.m_items) {
            ret.append(item->toQDspx());
        }
        return ret;
    }

    void AnchorNodeSequence::fromQDspx(const QList<QDspx::AnchorNode> &nodes) {
        while (size()) {
            removeItem(firstItem());
        }
        for (const auto &node : nodes) {
            auto item = model()->createAnchorNode();
            item->fromQDspx(node);
            insertItem(item);
        }
    }

    void AnchorNodeSequence::handleInsertIntoSequenceContainer(Handle entity) {
        Q_D(AnchorNodeSequence);
        d->handleInsertIntoSequenceContainer(entity);
    }

    void AnchorNodeSequence::handleTakeFromSequenceContainer(Handle takenEntity, Handle entity) {
        Q_D(AnchorNodeSequence);
        d->handleTakeFromSequenceContainer(takenEntity, entity);
    }

    ParamCurveAnchor *AnchorNodeSequence::paramCurveAnchor() const {
        Q_D(const AnchorNodeSequence);
        return d->paramCurveAnchor;
    }



}

#include "moc_AnchorNodeSequence.cpp"
