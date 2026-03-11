#include "PhonemeSequence.h"
#include "PhonemeSequence_p.h"

#include <QJSEngine>
#include <QJSValue>

#include <opendspx/phoneme.h>

#include <dspxmodel/Phoneme.h>
#include <dspxmodel/PhonemeInfo.h>
#include <dspxmodel/ModelStrategy.h>
#include <dspxmodel/private/Model_p.h>
#include <dspxmodel/private/PointSequenceData_p.h>

namespace dspx {

    PhonemeSequence::PhonemeSequence(PhonemeInfo *phonemeInfo, Handle handle, Model *model) : EntityObject(handle, model), d_ptr(new PhonemeSequencePrivate) {
        Q_D(PhonemeSequence);
        Q_ASSERT(model->strategy()->getEntityType(handle) == ModelStrategy::ES_Phonemes);
        d->q_ptr = this;
        d->pModel = ModelPrivate::get(model);
        d->phonemeInfo = phonemeInfo;

        d->init(model->strategy()->getEntitiesFromSequenceContainer(handle));
    }

    PhonemeSequence::~PhonemeSequence() = default;

    int PhonemeSequence::size() const {
        Q_D(const PhonemeSequence);
        return d->container.size();
    }

    Phoneme *PhonemeSequence::firstItem() const {
        Q_D(const PhonemeSequence);
        return d->firstItem;
    }

    Phoneme *PhonemeSequence::lastItem() const {
        Q_D(const PhonemeSequence);
        return d->lastItem;
    }

    Phoneme *PhonemeSequence::previousItem(Phoneme *item) const {
        Q_D(const PhonemeSequence);
        return d->container.previousItem(item);
    }

    Phoneme *PhonemeSequence::nextItem(Phoneme *item) const {
        Q_D(const PhonemeSequence);
        return d->container.nextItem(item);
    }

    QList<Phoneme *> PhonemeSequence::slice(int position, int length) const {
        Q_D(const PhonemeSequence);
        return d->container.slice(position, length);
    }

    bool PhonemeSequence::contains(Phoneme *item) const {
        Q_D(const PhonemeSequence);
        return d->container.contains(item);
    }

    bool PhonemeSequence::insertItem(Phoneme *item) {
        Q_D(PhonemeSequence);
        return d->pModel->strategy->insertIntoSequenceContainer(handle(), item->handle());
    }

    bool PhonemeSequence::removeItem(Phoneme *item) {
        Q_D(PhonemeSequence);
        return d->pModel->strategy->takeFromSequenceContainer(handle(), item->handle());
    }

    QList<QDspx::Phoneme> PhonemeSequence::toQDspx() const {
        Q_D(const PhonemeSequence);
        QList<QDspx::Phoneme> ret;
        ret.reserve(d->container.size());
        for (const auto &[_, item] : d->container.m_items) {
            ret.append(item->toQDspx());
        }
        return ret;
    }

    void PhonemeSequence::fromQDspx(const QList<QDspx::Phoneme> &phonemeList) {
        while (size()) {
            removeItem(firstItem());
        }
        for (const auto &phoneme : phonemeList) {
            auto item = model()->createPhoneme();
            item->fromQDspx(phoneme);
            insertItem(item);
        }
    }

    void PhonemeSequence::handleInsertIntoSequenceContainer(Handle entity) {
        Q_D(PhonemeSequence);
        d->handleInsertIntoSequenceContainer(entity);
    }

    void PhonemeSequence::handleTakeFromSequenceContainer(Handle takenEntity, Handle entity) {
        Q_D(PhonemeSequence);
        d->handleTakeFromSequenceContainer(takenEntity, entity);
    }

    PhonemeInfo *PhonemeSequence::phonemeInfo() const {
        Q_D(const PhonemeSequence);
        return d->phonemeInfo;
    }

}

#include "moc_PhonemeSequence.cpp"
