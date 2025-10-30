#include "AnchorNode.h"

#include <QVariant>
#include <QJSEngine>

#include <opendspx/anchornode.h>

#include <dspxmodel/Model.h>
#include <dspxmodel/ModelStrategy.h>

namespace dspx {

    class AnchorNodePrivate {
        Q_DECLARE_PUBLIC(AnchorNode)
    public:
        AnchorNode *q_ptr;
        AnchorNode::InterpolationMode interp;
        int x;
        int y;

        void setInterpUnchecked(AnchorNode::InterpolationMode interp_);
        void setInterp(AnchorNode::InterpolationMode interp_);
        void setXUnchecked(int x_);
        void setX(int x_);
    };

    void AnchorNodePrivate::setInterpUnchecked(AnchorNode::InterpolationMode interp_) {
        Q_Q(AnchorNode);
        q->model()->strategy()->setEntityProperty(q->handle(), ModelStrategy::P_Type, QVariant::fromValue(interp_));
    }

    void AnchorNodePrivate::setInterp(AnchorNode::InterpolationMode interp_) {
        setInterpUnchecked(interp_);
    }

    void AnchorNodePrivate::setXUnchecked(int x_) {
        Q_Q(AnchorNode);
        q->model()->strategy()->setEntityProperty(q->handle(), ModelStrategy::P_Position, x_);
    }

    void AnchorNodePrivate::setX(int x_) {
        Q_Q(AnchorNode);
        if (auto engine = qjsEngine(q); engine && x_ < 0) {
            engine->throwError(QJSValue::RangeError, QStringLiteral("Position must be greater or equal to 0"));
            return;
        }
        setXUnchecked(x_);
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
        d->setInterp(interp);
    }

    int AnchorNode::x() const {
        Q_D(const AnchorNode);
        return d->x;
    }

    void AnchorNode::setX(int x) {
        Q_D(AnchorNode);
        Q_ASSERT(x >= 0);
        d->setX(x);
    }

    int AnchorNode::y() const {
        Q_D(const AnchorNode);
        return d->y;
    }

    void AnchorNode::setY(int y) {
        Q_D(AnchorNode);
        model()->strategy()->setEntityProperty(handle(), ModelStrategy::P_Position, y);
    }

    QDspx::AnchorNode AnchorNode::toQDspx() const {
        Q_D(const AnchorNode);
        return QDspx::AnchorNode {
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