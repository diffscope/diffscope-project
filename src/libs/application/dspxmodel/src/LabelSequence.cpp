#include "LabelSequence.h"

#include <dspxmodel/private/PointSequenceContainer_p.h>
#include <dspxmodel/private/Model_p.h>
#include <dspxmodel/Label.h>
#include <dspxmodel/ModelStrategy.h>

namespace dspx {

    class LabelSequencePrivate {
        Q_DECLARE_PUBLIC(LabelSequence)
    public:
        LabelSequence *q_ptr;
        ModelPrivate *pModel;
        PointSequenceContainer<Label> container;
        Label *firstItem{};
        Label *lastItem{};

        Label *getItem(Handle handle, bool create) const;

        void insertItem(Label *item, int position);
        void removeItem(Label *item);
        void updateFirstAndLastItem();
    };

    Label *LabelSequencePrivate::getItem(Handle handle, bool create) const {
        if (auto object = pModel->mapToObject(handle)) {
            auto label = qobject_cast<Label *>(object);
            Q_ASSERT(label);
            return label;
        }
        if (create) {
            return pModel->createObject<Label>(handle);
        }
        Q_UNREACHABLE();
    }

    void LabelSequencePrivate::insertItem(Label *item, int position) {
        Q_Q(LabelSequence);
        bool containsItem = container.contains(item);
        if (!containsItem) {
            Q_EMIT q->itemAboutToInsert(item);
        }
        container.insertItem(item, position);
        if (!containsItem) {
            updateFirstAndLastItem();
            Q_EMIT q->itemInserted(item);
            Q_EMIT q->sizeChanged(container.size());
        }
    }

    void LabelSequencePrivate::removeItem(Label *item) {
        Q_Q(LabelSequence);
        Q_EMIT q->itemAboutToRemove(item);
        container.removeItem(item);
        updateFirstAndLastItem();
        Q_EMIT q->itemRemoved(item);
        Q_EMIT q->sizeChanged(container.size());
    }

    void LabelSequencePrivate::updateFirstAndLastItem() {
        Q_Q(LabelSequence);
        if (auto a = container.firstItem(); firstItem != a) {
            firstItem = a;
            Q_EMIT q->firstItemChanged(firstItem);
        }
        if (auto a = container.lastItem(); lastItem != a) {
            lastItem = a;
            Q_EMIT q->lastItemChanged(lastItem);
        }
    }

    LabelSequence::LabelSequence(Handle handle, Model *model) : EntityObject(handle, model), d_ptr(new LabelSequencePrivate) {
        Q_D(LabelSequence);
        d->q_ptr = this;
        d->pModel = ModelPrivate::get(model);
    }

    LabelSequence::~LabelSequence() = default;

    int LabelSequence::size() const {
        Q_D(const LabelSequence);
        return d->container.size();
    }

    Label *LabelSequence::firstItem() const {
        Q_D(const LabelSequence);
        return d->firstItem;
    }

    Label *LabelSequence::lastItem() const {
        Q_D(const LabelSequence);
        return d->lastItem;
    }

    Label *LabelSequence::previousItem(Label *item) const {
        Q_D(const LabelSequence);
        return d->container.previousItem(item);
    }

    Label *LabelSequence::nextItem(Label *item) const {
        Q_D(const LabelSequence);
        return d->container.nextItem(item);
    }

    QList<Label *> LabelSequence::slice(int position, int length) const {
        Q_D(const LabelSequence);
        return d->container.slice(position, length);
    }

    bool LabelSequence::contains(Label *item) const {
        Q_D(const LabelSequence);
        return d->container.contains(item);
    }

    void LabelSequence::insertItem(Label *item) {
        Q_D(LabelSequence);
        d->pModel->strategy->insertIntoSequenceContainer(handle(), item->handle());
    }

    void LabelSequence::removeItem(Label *item) {
        Q_D(LabelSequence);
        d->pModel->strategy->takeFromSequenceContainer(handle(), item->handle());
    }

    void LabelSequence::handleInsertIntoSequenceContainer(Handle entity) {
        Q_D(LabelSequence);
        auto label = d->getItem(entity, true);
        connect(label, &Label::posChanged, this, [=](int pos) {
            d->insertItem(label, pos);
        });
        connect(label, &Label::destroyed, this, [=] {
            d->removeItem(label);
        });
        d->insertItem(label, label->pos());
    }

    void LabelSequence::handleTakeFromSequenceContainer(Handle takenEntity, Handle entity) {
        Q_D(LabelSequence);
        auto label = d->getItem(takenEntity, false);
        disconnect(label, nullptr, this, nullptr);
        d->removeItem(label);
    }

}

#include "moc_LabelSequence.cpp"