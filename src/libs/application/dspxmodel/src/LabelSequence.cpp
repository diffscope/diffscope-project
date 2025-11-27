#include "LabelSequence.h"
#include "LabelSequence_p.h"

#include <QJSEngine>
#include <QJSValue>

#include <opendspx/label.h>

#include <dspxmodel/Label.h>
#include <dspxmodel/ModelStrategy.h>
#include <dspxmodel/private/Model_p.h>
#include <dspxmodel/private/PointSequenceData_p.h>

namespace dspx {

    LabelSequence::LabelSequence(Handle handle, Model *model) : EntityObject(handle, model), d_ptr(new LabelSequencePrivate) {
        Q_D(LabelSequence);
        Q_ASSERT(model->strategy()->getEntityType(handle) == ModelStrategy::ES_Labels);
        d->q_ptr = this;
        d->pModel = ModelPrivate::get(model);
        connect(this, &LabelSequence::itemInserted, this, [=](Label *item) {
            LabelPrivate::setLabelSequence(item, this);
        });
        connect(this, &LabelSequence::itemRemoved, this, [=](Label *item) {
            LabelPrivate::setLabelSequence(item, nullptr);
        });
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

    bool LabelSequence::insertItem(Label *item) {
        Q_D(LabelSequence);
        return d->pModel->strategy->insertIntoSequenceContainer(handle(), item->handle());
    }

    bool LabelSequence::removeItem(Label *item) {
        Q_D(LabelSequence);
        return d->pModel->strategy->takeFromSequenceContainer(handle(), item->handle());
    }

    QList<QDspx::Label> LabelSequence::toQDspx() const {
        Q_D(const LabelSequence);
        QList<QDspx::Label> ret;
        ret.reserve(d->container.size());
        for (const auto &[_, item] : d->container.m_items) {
            ret.append(item->toQDspx());
        }
        return ret;
    }

    void LabelSequence::fromQDspx(const QList<QDspx::Label> &labels) {
        while (size()) {
            removeItem(firstItem());
        }
        for (const auto &label : labels) {
            auto item = model()->createLabel();
            item->fromQDspx(label);
            insertItem(item);
        }
    }

    void LabelSequence::handleInsertIntoSequenceContainer(Handle entity) {
        Q_D(LabelSequence);
        d->handleInsertIntoSequenceContainer(entity);
    }

    void LabelSequence::handleTakeFromSequenceContainer(Handle takenEntity, Handle entity) {
        Q_D(LabelSequence);
        d->handleTakeFromSequenceContainer(takenEntity, entity);
    }

}

#include "moc_LabelSequence.cpp"
