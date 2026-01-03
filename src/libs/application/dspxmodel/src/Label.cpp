#include "Label.h"
#include "Label_p.h"

#include <QJSEngine>
#include <QVariant>

#include <opendspx/label.h>

#include <dspxmodel/LabelSequence.h>
#include <dspxmodel/Model.h>
#include <dspxmodel/ModelStrategy.h>

namespace dspx {

    void LabelPrivate::setPosUnchecked(int pos_) {
        Q_Q(Label);
        q->model()->strategy()->setEntityProperty(q->handle(), ModelStrategy::P_Position, pos_);
    }

    void LabelPrivate::setPos(int pos_) {
        Q_Q(Label);
        if (pos_ < -0) {
            if (auto engine = qjsEngine(q))
                engine->throwError(QJSValue::RangeError, QStringLiteral("Pos must be greater or equal to 0"));
            return;
        }
        setPosUnchecked(pos_);
    }

    void LabelPrivate::setLabelSequence(Label *item, LabelSequence *labelSequence) {
        auto d = item->d_func();
        if (d->labelSequence != labelSequence) {
            d->labelSequence = labelSequence;
            Q_EMIT item->labelSequenceChanged();
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
        d->setPosUnchecked(pos);
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
