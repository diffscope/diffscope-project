#include "LabelSequence.h"

#include <dspxmodel/private/PointSequenceContainer_p.h>
#include <dspxmodel/private/Model_p.h>
#include <dspxmodel/Label.h>

namespace dspx {

    class LabelSequencePrivate {
        Q_DECLARE_PUBLIC(LabelSequence)
    public:
        LabelSequence *q_ptr;
        ModelPrivate *pModel;
        PointSequenceContainer<Label> container;

        Label *getLabel(Handle handle, bool create) const;

        void insertItem(Label *item, int position);
        void removeItem(Label *item);
    };

    void LabelSequencePrivate::insertItem(Label *item, int position) {
        container.insertItem(item, position);
    }

    void LabelSequencePrivate::removeItem(Label *item) {
        container.removeItem(item);
    }

    Label *LabelSequencePrivate::getLabel(Handle handle, bool create) const {
        if (auto object = pModel->mapToObject(handle)) {
            auto label = qobject_cast<Label *>(object);
            Q_ASSERT(label);
            return label;
        }
        if (create) {
            return pModel->createLabel(handle);
        }
        Q_UNREACHABLE();
    }

    LabelSequence::LabelSequence(Handle handle, Model *model) : EntityObject(handle, model), d_ptr(new LabelSequencePrivate) {
        Q_D(LabelSequence);
        d->q_ptr = this;
        d->pModel = ModelPrivate::get(model);
    }

    LabelSequence::~LabelSequence() = default;

    void LabelSequence::handleInsertIntoSequenceContainer(Handle entity) {
        Q_D(LabelSequence);
        auto label = d->getLabel(entity, true);
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
        auto label = d->getLabel(takenEntity, false);
        disconnect(label, nullptr, this, nullptr);
        d->removeItem(label);
    }

}