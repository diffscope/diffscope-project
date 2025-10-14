#include "Label.h"

#include <QVariant>

#include <opendspx/qdspxmodel.h>

#include <dspxmodel/Model.h>
#include <dspxmodel/ModelStrategy.h>

namespace dspx {

    class LabelPrivate {
        Q_DECLARE_PUBLIC(Label)
    public:
        Label *q_ptr;
        int pos;
        QString text;
    };

    Label::Label(Handle handle, Model *model) : EntityObject(handle, model), d_ptr(new LabelPrivate) {
        Q_D(Label);
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

    QDspx::Label Label::toQDspx() const {
        return QDspx::Label {
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