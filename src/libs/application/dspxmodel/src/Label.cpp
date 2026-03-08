#include "Label.h"
#include "Label_p.h"
#include <QVariant>

#include <opendspx/label.h>

#include <dspxmodel/LabelSequence.h>
#include <dspxmodel/Model.h>
#include <dspxmodel/ModelStrategy.h>

namespace dspx {



    void LabelPrivate::setLabelSequence(Label *item, LabelSequence *labelSequence) {
        auto d = item->d_func();
        if (d->labelSequence != labelSequence) {
            d->labelSequence = labelSequence;
            Q_EMIT item->labelSequenceChanged();
        }
    }

    void LabelPrivate::setPreviousItem(Label *item, Label *previousItem) {
        auto d = item->d_func();
        if (d->previousItem != previousItem) {
            d->previousItem = previousItem;
            Q_EMIT item->previousItemChanged();
        }
    }

    void LabelPrivate::setNextItem(Label *item, Label *nextItem) {
        auto d = item->d_func();
        if (d->nextItem != nextItem) {
            d->nextItem = nextItem;
            Q_EMIT item->nextItemChanged();
        }
    }

    Label::Label(Handle handle, Model *model) : EntityObject(handle, model), d_ptr(new LabelPrivate) {
        Q_D(Label);
        Q_ASSERT(model->strategy()->getEntityType(handle) == ModelStrategy::EI_Label);
        d->q_ptr = this;
        d->pos = model->strategy()->getEntityProperty(handle, ModelStrategy::P_Position).toInt();
        d->text = model->strategy()->getEntityProperty(handle, ModelStrategy::P_Text).toString();
    }

    Label::~Label() = default;

    int Label::pos() const {
        Q_D(const Label);
        return d->pos;
    }

    void Label::setPos(int pos) {
        Q_D(Label);
        Q_ASSERT(pos >= 0);
        model()->strategy()->setEntityProperty(handle(), ModelStrategy::P_Position, pos);
    }

    QString Label::text() const {
        Q_D(const Label);
        return d->text;
    }

    void Label::setText(const QString &text) {
        Q_D(Label);
        model()->strategy()->setEntityProperty(handle(), ModelStrategy::P_Text, text);
    }

    LabelSequence *Label::labelSequence() const {
        Q_D(const Label);
        return d->labelSequence;
    }

    Label *Label::previousItem() const {
        Q_D(const Label);
        return d->previousItem;
    }

    Label *Label::nextItem() const {
        Q_D(const Label);
        return d->nextItem;
    }

    QDspx::Label Label::toQDspx() const {
        return {
            .pos = pos(),
            .text = text(),
        };
    }

    void Label::fromQDspx(const QDspx::Label &label) {
        setPos(label.pos);
        setText(label.text);
    }

    void Label::handleSetEntityProperty(int property, const QVariant &value) {
        Q_D(Label);
        switch (property) {
            case ModelStrategy::P_Position: {
                d->pos = value.toInt();
                Q_EMIT posChanged(d->pos);
                break;
            }
            case ModelStrategy::P_Text: {
                d->text = value.toString();
                Q_EMIT textChanged(d->text);
                break;
            }
            default:
                Q_UNREACHABLE();
        }
    }

}

#include "moc_Label.cpp"
