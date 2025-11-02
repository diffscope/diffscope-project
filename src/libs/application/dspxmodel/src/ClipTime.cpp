#include "ClipTime.h"

#include <QVariant>
#include <QJSEngine>

#include <opendspx/cliptime.h>

#include <dspxmodel/private/Model_p.h>
#include <dspxmodel/ModelStrategy.h>

namespace dspx {

    class ClipTimePrivate {
        Q_DECLARE_PUBLIC(ClipTime)
    public:
        ClipTime *q_ptr;
        ModelPrivate *pModel;
        Handle handle;

        int start;
        int length;
        int clipStart;
        int clipLen;

        void setLengthUnchecked(int length_);
        void setLength(int length_);
        void setClipStartUnchecked(int clipStart_);
        void setClipStart(int clipStart_);
        void setClipLenUnchecked(int clipLen_);
        void setClipLen(int clipLen_);
    };

    void ClipTimePrivate::setLengthUnchecked(int length_) {
        Q_Q(ClipTime);
        pModel->strategy->setEntityProperty(handle, ModelStrategy::P_Length, length_);
    }

    void ClipTimePrivate::setLength(int length_) {
        Q_Q(ClipTime);
        if (auto engine = qjsEngine(q); engine && length_ < 0) {
            engine->throwError(QJSValue::RangeError, QStringLiteral("Length must be greater than or equal to 0"));
            return;
        }
        setLengthUnchecked(length_);
    }

    void ClipTimePrivate::setClipStartUnchecked(int clipStart_) {
        Q_Q(ClipTime);
        pModel->strategy->setEntityProperty(handle, ModelStrategy::P_ClipStart, clipStart_);
    }

    void ClipTimePrivate::setClipStart(int clipStart_) {
        Q_Q(ClipTime);
        if (auto engine = qjsEngine(q); engine && clipStart_ < 0) {
            engine->throwError(QJSValue::RangeError, QStringLiteral("ClipStart must be greater than or equal to 0"));
            return;
        }
        setClipStartUnchecked(clipStart_);
    }

    void ClipTimePrivate::setClipLenUnchecked(int clipLen_) {
        Q_Q(ClipTime);
        pModel->strategy->setEntityProperty(handle, ModelStrategy::P_ClipLength, clipLen_);
    }

    void ClipTimePrivate::setClipLen(int clipLen_) {
        Q_Q(ClipTime);
        if (auto engine = qjsEngine(q); engine && clipLen_ < 0) {
            engine->throwError(QJSValue::RangeError, QStringLiteral("ClipLen must be greater than or equal to 0"));
            return;
        }
        setClipLenUnchecked(clipLen_);
    }

    ClipTime::ClipTime(Handle handle, Model *model) : QObject(model), d_ptr(new ClipTimePrivate) {
        Q_D(ClipTime);
        d->q_ptr = this;
        d->pModel = ModelPrivate::get(model);
        d->handle = handle;
        d->start = d->pModel->strategy->getEntityProperty(handle, ModelStrategy::P_Position).toInt();
        d->length = d->pModel->strategy->getEntityProperty(handle, ModelStrategy::P_Length).toInt();
        d->clipStart = d->pModel->strategy->getEntityProperty(handle, ModelStrategy::P_ClipStart).toInt();
        d->clipLen = d->pModel->strategy->getEntityProperty(handle, ModelStrategy::P_ClipLength).toInt();
    }

    ClipTime::~ClipTime() = default;

    int ClipTime::start() const {
        Q_D(const ClipTime);
        return d->start;
    }

    void ClipTime::setStart(int start) {
        Q_D(ClipTime);
        d->pModel->strategy->setEntityProperty(d->handle, ModelStrategy::P_Position, start);
    }

    int ClipTime::length() const {
        Q_D(const ClipTime);
        return d->length;
    }

    void ClipTime::setLength(int length) {
        Q_D(ClipTime);
        Q_ASSERT(length >= 0);
        d->setLengthUnchecked(length);
    }

    int ClipTime::clipStart() const {
        Q_D(const ClipTime);
        return d->clipStart;
    }

    void ClipTime::setClipStart(int clipStart) {
        Q_D(ClipTime);
        Q_ASSERT(clipStart >= 0);
        d->setClipStartUnchecked(clipStart);
    }

    int ClipTime::clipLen() const {
        Q_D(const ClipTime);
        return d->clipLen;
    }

    void ClipTime::setClipLen(int clipLen) {
        Q_D(ClipTime);
        Q_ASSERT(clipLen >= 0);
        d->setClipLenUnchecked(clipLen);
    }

    QDspx::ClipTime ClipTime::toQDspx() const {
        return {
            .start = start(),
            .length = length(),
            .clipStart = clipStart(),
            .clipLen = clipLen()
        };
    }

    void ClipTime::fromQDspx(const QDspx::ClipTime &clipTime) {
        setStart(clipTime.start);
        setLength(clipTime.length);
        setClipStart(clipTime.clipStart);
        setClipLen(clipTime.clipLen);
    }

    void ClipTime::handleProxySetEntityProperty(int property, const QVariant &value) {
        Q_D(ClipTime);
        switch (property) {
            case ModelStrategy::P_Position: {
                d->start = value.toInt();
                Q_EMIT startChanged(d->start);
                break;
            }
            case ModelStrategy::P_Length: {
                d->length = value.toInt();
                Q_EMIT lengthChanged(d->length);
                break;
            }
            case ModelStrategy::P_ClipStart: {
                d->clipStart = value.toInt();
                Q_EMIT clipStartChanged(d->clipStart);
                break;
            }
            case ModelStrategy::P_ClipLength: {
                d->clipLen = value.toInt();
                Q_EMIT clipLenChanged(d->clipLen);
                break;
            }
            default:
                Q_UNREACHABLE();
        }
    }

}

#include "moc_ClipTime.cpp"