#include "TimeSignatureSequence.h"
#include "TimeSignatureSequence_p.h"

#include <QJSEngine>
#include <QJSValue>

#include <opendspx/timesignature.h>

#include <dspxmodel/ModelStrategy.h>
#include <dspxmodel/TimeSignature.h>
#include <dspxmodel/private/Model_p.h>
#include <dspxmodel/private/PointSequenceData_p.h>

namespace dspx {

    TimeSignatureSequence::TimeSignatureSequence(Handle handle, Model *model) : EntityObject(handle, model), d_ptr(new TimeSignatureSequencePrivate) {
        Q_D(TimeSignatureSequence);
        Q_ASSERT(model->strategy()->getEntityType(handle) == ModelStrategy::ES_TimeSignatures);
        d->q_ptr = this;
        d->pModel = ModelPrivate::get(model);

        d->init(model->strategy()->getEntitiesFromSequenceContainer(handle));
    }

    TimeSignatureSequence::~TimeSignatureSequence() = default;

    int TimeSignatureSequence::size() const {
        Q_D(const TimeSignatureSequence);
        return d->container.size();
    }

    TimeSignature *TimeSignatureSequence::firstItem() const {
        Q_D(const TimeSignatureSequence);
        return d->firstItem;
    }

    TimeSignature *TimeSignatureSequence::lastItem() const {
        Q_D(const TimeSignatureSequence);
        return d->lastItem;
    }

    TimeSignature *TimeSignatureSequence::previousItem(TimeSignature *item) const {
        Q_D(const TimeSignatureSequence);
        return d->container.previousItem(item);
    }

    TimeSignature *TimeSignatureSequence::nextItem(TimeSignature *item) const {
        Q_D(const TimeSignatureSequence);
        return d->container.nextItem(item);
    }

    QList<TimeSignature *> TimeSignatureSequence::slice(int position, int length) const {
        Q_D(const TimeSignatureSequence);
        return d->container.slice(position, length);
    }

    bool TimeSignatureSequence::contains(TimeSignature *item) const {
        Q_D(const TimeSignatureSequence);
        return d->container.contains(item);
    }

    bool TimeSignatureSequence::insertItem(TimeSignature *item) {
        Q_D(TimeSignatureSequence);
        return d->pModel->strategy->insertIntoSequenceContainer(handle(), item->handle());
    }

    bool TimeSignatureSequence::removeItem(TimeSignature *item) {
        Q_D(TimeSignatureSequence);
        return d->pModel->strategy->takeFromSequenceContainer(handle(), item->handle());
    }

    std::vector<opendspx::TimeSignature> TimeSignatureSequence::toOpenDspx() const {
        Q_D(const TimeSignatureSequence);
        std::vector<opendspx::TimeSignature> ret;
        ret.reserve(d->container.size());
        for (const auto &[_, item] : d->container.m_items) {
            ret.push_back(item->toOpenDspx());
        }
        return ret;
    }

    void TimeSignatureSequence::fromOpenDspx(const std::vector<opendspx::TimeSignature> &timeSignatures) {
        while (size()) {
            removeItem(firstItem());
        }
        for (const auto &timeSignature : timeSignatures) {
            auto item = model()->createTimeSignature();
            item->fromOpenDspx(timeSignature);
            insertItem(item);
        }
    }

    void TimeSignatureSequence::handleInsertIntoSequenceContainer(Handle entity) {
        Q_D(TimeSignatureSequence);
        d->handleInsertIntoSequenceContainer(entity);
    }

    void TimeSignatureSequence::handleTakeFromSequenceContainer(Handle takenEntity, Handle entity) {
        Q_D(TimeSignatureSequence);
        d->handleTakeFromSequenceContainer(takenEntity, entity);
    }

}

#include "moc_TimeSignatureSequence.cpp"
