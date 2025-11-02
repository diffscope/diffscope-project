#include "TimeSignatureSequence.h"

#include <QJSEngine>
#include <QJSValue>

#include <opendspx/timesignature.h>

#include <dspxmodel/ModelStrategy.h>
#include <dspxmodel/TimeSignature.h>
#include <dspxmodel/private/Model_p.h>
#include <dspxmodel/private/PointSequenceContainer_p.h>
#include <dspxmodel/private/PointSequenceData_p.h>

namespace dspx {

    class TimeSignatureSequencePrivate : public PointSequenceData<TimeSignatureSequence, TimeSignature, &TimeSignature::index, &TimeSignature::indexChanged> {
        Q_DECLARE_PUBLIC(TimeSignatureSequence)
    };

    TimeSignatureSequence::TimeSignatureSequence(Handle handle, Model *model) : EntityObject(handle, model), d_ptr(new TimeSignatureSequencePrivate) {
        Q_D(TimeSignatureSequence);
        Q_ASSERT(model->strategy()->getEntityType(handle) == ModelStrategy::ES_TimeSignatures);
        d->q_ptr = this;
        d->pModel = ModelPrivate::get(model);
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

    QList<QDspx::TimeSignature> TimeSignatureSequence::toQDspx() const {
        Q_D(const TimeSignatureSequence);
        QList<QDspx::TimeSignature> ret;
        ret.reserve(d->container.size());
        for (const auto &[_, item] : d->container.m_items) {
            ret.append(item->toQDspx());
        }
        return ret;
    }

    void TimeSignatureSequence::fromQDspx(const QList<QDspx::TimeSignature> &timeSignatures) {
        while (size()) {
            removeItem(firstItem());
        }
        for (const auto &timeSignature : timeSignatures) {
            auto item = model()->createTimeSignature();
            item->fromQDspx(timeSignature);
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
