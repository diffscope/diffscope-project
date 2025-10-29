#include "PhonemeList.h"

#include "ModelStrategy.h"

#include <opendspx/phoneme.h>

#include <dspxmodel/private/Model_p.h>
#include <dspxmodel/private/ListData_p.h>
#include <dspxmodel/Phoneme.h>

namespace dspx {

    class PhonemeListPrivate : public ListData<PhonemeList, Phoneme> {
        Q_DECLARE_PUBLIC(PhonemeList)
    };

    PhonemeList::PhonemeList(Handle handle, Model *model) : EntityObject(handle, model), d_ptr(new PhonemeListPrivate) {
        Q_D(PhonemeList);
        Q_ASSERT(model->strategy()->getEntityType(handle) == ModelStrategy::EL_Phonemes);
        d->q_ptr = this;
        d->pModel = ModelPrivate::get(model);
    }

    PhonemeList::~PhonemeList() = default;

    int PhonemeList::size() const {
        Q_D(const PhonemeList);
        return d->size();
    }

    QList<Phoneme *> PhonemeList::items() const {
        Q_D(const PhonemeList);
        return d->items();
    }

    bool PhonemeList::insertItem(int index, Phoneme *item) {
        Q_D(PhonemeList);
        return d->pModel->strategy->insertIntoListContainer(handle(), item->handle(), index);
    }

    Phoneme *PhonemeList::removeItem(int index) {
        Q_D(PhonemeList);
        auto takenEntityHandle = d->pModel->strategy->takeFromListContainer(handle(), index);
        return d->getItem(takenEntityHandle, false);
    }

    Phoneme *PhonemeList::item(int index) const {
        Q_D(const PhonemeList);
        return d->item(index);
    }

    bool PhonemeList::rotate(int leftIndex, int middleIndex, int rightIndex) {
        Q_D(PhonemeList);
        return d->pModel->strategy->rotateListContainer(handle(), leftIndex, middleIndex, rightIndex);
    }

    QList<QDspx::Phoneme> PhonemeList::toQDspx() const {
        Q_D(const PhonemeList);
        QList<QDspx::Phoneme> ret;
        for (const auto &phoneme : d->itemList) {
            ret.append(phoneme->toQDspx());
        }
        return ret;
    }

    void PhonemeList::fromQDspx(const QList<QDspx::Phoneme> &phonemeList) {
        while (size() > 0) {
            removeItem(0);
        }
        for (const auto &phonemeData : phonemeList) {
            auto phoneme = model()->createPhoneme();
            phoneme->fromQDspx(phonemeData);
            insertItem(size(), phoneme);
        }
    }

    void PhonemeList::handleInsertIntoListContainer(Handle entity, int index) {
        Q_D(PhonemeList);
        d->handleInsertIntoListContainer(entity, index);
    }

    void PhonemeList::handleTakeFromListContainer(Handle takenEntity, int index) {
        Q_D(PhonemeList);
        d->handleTakeFromListContainer(takenEntity, index);
    }

    void PhonemeList::handleRotateListContainer(int leftIndex, int middleIndex, int rightIndex) {
        Q_D(PhonemeList);
        d->handleRotateListContainer(leftIndex, middleIndex, rightIndex);
    }

}

#include "moc_PhonemeList.cpp"