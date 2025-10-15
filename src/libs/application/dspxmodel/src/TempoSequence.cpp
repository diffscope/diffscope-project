#include "TempoSequence.h"

#include <QJSValue>
#include <QJSEngine>

#include <opendspx/qdspxmodel.h>

#include <dspxmodel/private/PointSequenceContainer_p.h>
#include <dspxmodel/private/Model_p.h>
#include <dspxmodel/Tempo.h>
#include <dspxmodel/ModelStrategy.h>
#include <dspxmodel/private/PointSequenceData_p.h>

namespace dspx {

    class TempoSequencePrivate : public PointSequenceData<TempoSequence, Tempo, &Tempo::pos, &Tempo::posChanged> {
        Q_DECLARE_PUBLIC(TempoSequence)
    };

    TempoSequence::TempoSequence(Handle handle, Model *model) : EntityObject(handle, model), d_ptr(new TempoSequencePrivate) {
        Q_D(TempoSequence);
        d->q_ptr = this;
        d->pModel = ModelPrivate::get(model);
    }

    TempoSequence::~TempoSequence() = default;

    int TempoSequence::size() const {
        Q_D(const TempoSequence);
        return d->container.size();
    }

    Tempo *TempoSequence::firstItem() const {
        Q_D(const TempoSequence);
        return d->firstItem;
    }

    Tempo *TempoSequence::lastItem() const {
        Q_D(const TempoSequence);
        return d->lastItem;
    }

    Tempo *TempoSequence::previousItem(Tempo *item) const {
        Q_D(const TempoSequence);
        return d->container.previousItem(item);
    }

    Tempo *TempoSequence::nextItem(Tempo *item) const {
        Q_D(const TempoSequence);
        return d->container.nextItem(item);
    }

    QList<Tempo *> TempoSequence::slice(int position, int length) const {
        Q_D(const TempoSequence);
        return d->container.slice(position, length);
    }

    bool TempoSequence::contains(Tempo *item) const {
        Q_D(const TempoSequence);
        return d->container.contains(item);
    }

    bool TempoSequence::insertItem(Tempo *item) {
        Q_D(TempoSequence);
        return d->pModel->strategy->insertIntoSequenceContainer(handle(), item->handle());
    }

    bool TempoSequence::removeItem(Tempo *item) {
        Q_D(TempoSequence);
        return d->pModel->strategy->takeFromSequenceContainer(handle(), item->handle());
    }

    QList<QDspx::Tempo> TempoSequence::toQDspx() const {
        Q_D(const TempoSequence);
        QList<QDspx::Tempo> ret;
        ret.reserve(d->container.size());
        for (const auto &[_, item] : d->container.m_items) {
            ret.append(item->toQDspx());
        }
        return ret;
    }

    void TempoSequence::fromQDspx(const QList<QDspx::Tempo> &tempos) {
        while (size()) {
            removeItem(firstItem());
        }
        for (const auto &tempo : tempos) {
            auto item = model()->createTempo();
            item->fromQDspx(tempo);
            insertItem(item);
        }
    }

    void TempoSequence::handleInsertIntoSequenceContainer(Handle entity) {
        Q_D(TempoSequence);
        d->handleInsertIntoSequenceContainer(entity);
    }

    void TempoSequence::handleTakeFromSequenceContainer(Handle takenEntity, Handle entity) {
        Q_D(TempoSequence);
        d->handleTakeFromSequenceContainer(takenEntity, entity);
    }

}

#include "moc_TempoSequence.cpp"