#include "AnchorNode.h"
#include "AnchorNode_p.h"

#include <QJSEngine>
#include <QVariant>

#include <opendspx/anchornode.h>

#include <dspxmodel/AnchorNodeSequence.h>
#include <dspxmodel/Model.h>
#include <dspxmodel/ModelStrategy.h>

namespace dspx {

    void AnchorNodePrivate::setInterpUnchecked(AnchorNode::InterpolationMode interp_) {
        Q_Q(AnchorNode);
        q->model()->strategy()->setEntityProperty(q->handle(), ModelStrategy::P_Type, QVariant::fromValue(interp_));
    }

    void AnchorNodePrivate::setInterp(AnchorNode::InterpolationMode interp_) {
        Q_Q(AnchorNode);
        if ((interp_ != AnchorNode::None && interp_ != AnchorNode::Linear && interp_ != AnchorNode::Hermite)) {
            if (auto engine = qjsEngine(q))
                engine->throwError(QJSValue::RangeError, QStringLiteral("Interpolation mode must be one of None, Linear, or Hermite"));
            return;
        }
        setInterpUnchecked(interp_);
    }

    void AnchorNodePrivate::setXUnchecked(int x_) {
        Q_Q(AnchorNode);
        q->model()->strategy()->setEntityProperty(q->handle(), ModelStrategy::P_Position, x_);
    }

    void AnchorNodePrivate::setX(int x_) {
        Q_Q(AnchorNode);
        if (x_ < 0) {
            if (auto engine = qjsEngine(q))
                engine->throwError(QJSValue::RangeError, QStringLiteral("Position must be greater or equal to 0"));
            return;
        }
        setXUnchecked(x_);
    }

    void AnchorNodePrivate::setAnchorNodeSequence(AnchorNode *item, AnchorNodeSequence *anchorNodeSequence) {
        auto d = item->d_func();
        if (d->anchorNodeSequence != anchorNodeSequence) {
            d->anchorNodeSequence = anchorNodeSequence;
            Q_EMIT item->anchorNodeSequenceChanged();
        }
    }

    AnchorNode::AnchorNode(Handle handle, Model *model)
        : EntityObject(handle, model), d_ptr(new AnchorNodePrivate) {
        Q_D(AnchorNode);
        Q_ASSERT(model->strategy()->getEntityType(handle) == ModelStrategy::EI_ParamCurveAnchorNode);
        d->q_ptr = this;
        d->interp = model->strategy()->getEntityProperty(handle, ModelStrategy::P_Type).value<InterpolationMode>();
        d->x = model->strategy()->getEntityProperty(handle, ModelStrategy::P_Position).toInt();
        d->y = model->strategy()->getEntityProperty(handle, ModelStrategy::P_Value).toInt();
    }

    AnchorNode::~AnchorNode() = default;

    AnchorNode::InterpolationMode AnchorNode::interp() const {
        Q_D(const AnchorNode);
        return d->interp;
    }

    void AnchorNode::setInterp(InterpolationMode interp) {
        Q_D(AnchorNode);
        Q_ASSERT(interp == None || interp == Linear || interp == Hermite);
        d->setInterpUnchecked(interp);
    }

    int AnchorNode::x() const {
        Q_D(const AnchorNode);
        return d->x;
    }

    void AnchorNode::setX(int x) {
        Q_D(AnchorNode);
        Q_ASSERT(x >= 0);
        d->setXUnchecked(x);
    }

    int AnchorNode::y() const {
        Q_D(const AnchorNode);
        return d->y;
    }

    void AnchorNode::setY(int y) {
        Q_D(AnchorNode);
        model()->strategy()->setEntityProperty(handle(), ModelStrategy::P_Value, y);
    }

    QDspx::AnchorNode AnchorNode::toQDspx() const {
        Q_D(const AnchorNode);
        return {
            .interp = static_cast<QDspx::AnchorNode::Interpolation>(d->interp),
            .x = d->x,
            .y = d->y,
        };
    }

    void AnchorNode::fromQDspx(const QDspx::AnchorNode &node) {
        Q_D(AnchorNode);
        setInterp(static_cast<InterpolationMode>(node.interp));
        setX(node.x);
        setY(node.y);
    }

    AnchorNodeSequence *AnchorNode::anchorNodeSequence() const {
        Q_D(const AnchorNode);
        return d->anchorNodeSequence;
    }

    void AnchorNode::handleSetEntityProperty(int property, const QVariant &value) {
        Q_D(AnchorNode);
        switch (property) {
            case ModelStrategy::P_Type: {
                d->interp = value.value<InterpolationMode>();
                Q_EMIT interpChanged(d->interp);
                break;
            }
            case ModelStrategy::P_Position: {
                d->x = value.toInt();
                Q_EMIT xChanged(d->x);
                break;
            }
            case ModelStrategy::P_Value: {
                d->y = value.toInt();
                Q_EMIT yChanged(d->y);
                break;
            }
            default:
                Q_UNREACHABLE();
        }
    }

}

#include "moc_AnchorNode.cpp"
