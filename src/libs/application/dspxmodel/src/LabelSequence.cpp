#include "LabelSequence.h"

#include <QJSValue>
#include <QJSEngine>

#include <opendspx/qdspxmodel.h>

#include <dspxmodel/private/PointSequenceContainer_p.h>
#include <dspxmodel/private/Model_p.h>
#include <dspxmodel/Label.h>
#include <dspxmodel/ModelStrategy.h>
#include <dspxmodel/private/PointSequenceData_p.h>

namespace dspx {

    class LabelSequencePrivate : public PointSequenceData<LabelSequence, Label, &Label::pos, &Label::posChanged> {
        Q_DECLARE_PUBLIC(LabelSequence)
    };

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