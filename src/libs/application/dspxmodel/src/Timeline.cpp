#include "Timeline.h"

#include <opendspx/timeline.h>

#include <dspxmodel/ModelStrategy.h>
#include <dspxmodel/LabelSequence.h>
#include <dspxmodel/KeySignatureSequence.h>
#include <dspxmodel/TempoSequence.h>
#include <dspxmodel/TimeSignatureSequence.h>
#include <dspxmodel/private/Model_p.h>

namespace dspx {

    class TimelinePrivate {
        Q_DECLARE_PUBLIC(Timeline)
    public:
        Timeline *q_ptr;
        ModelPrivate *pModel;
        Handle handle;

        bool loopEnabled{false};
        int loopStart{0};
        int loopLength{1920};
    };

    Timeline::Timeline(Model *model)
        : QObject(model), d_ptr(new TimelinePrivate) {
        Q_D(Timeline);
        d->q_ptr = this;
        d->pModel = ModelPrivate::get(model);
        d->handle = model->handle();
    }

    void Timeline::handleProxySetEntityProperty(int property, const QVariant &value) {
        Q_D(Timeline);
        switch (property) {
            case ModelStrategy::P_LoopEnabled:
                d->loopEnabled = value.toBool();
                Q_EMIT loopEnabledChanged(d->loopEnabled);
                break;
            case ModelStrategy::P_LoopStart:
                d->loopStart = value.toInt();
                Q_EMIT loopStartChanged(d->loopStart);
                break;
            case ModelStrategy::P_LoopLength:
                d->loopLength = value.toInt();
                Q_EMIT loopLengthChanged(d->loopLength);
                break;
            default:
                Q_UNREACHABLE();
        }
    }

    Timeline::~Timeline() = default;

    LabelSequence *Timeline::labels() const {
        Q_D(const Timeline);
        return d->pModel->labels;
    }

    KeySignatureSequence *Timeline::keySignatures() const {
        Q_D(const Timeline);
        return d->pModel->keySignatures;
    }

    TempoSequence *Timeline::tempos() const {
        Q_D(const Timeline);
        return d->pModel->tempos;
    }

    TimeSignatureSequence *Timeline::timeSignatures() const {
        Q_D(const Timeline);
        return d->pModel->timeSignatures;
    }

    bool Timeline::isLoopEnabled() const {
        Q_D(const Timeline);
        return d->loopEnabled;
    }

    void Timeline::setLoopEnabled(bool enabled) {
        Q_D(Timeline);
        d->pModel->strategy->setEntityProperty(d->handle, ModelStrategy::P_LoopEnabled, enabled);
    }

    int Timeline::loopStart() const {
        Q_D(const Timeline);
        return d->loopStart;
    }

    void Timeline::setLoopStart(int loopStart) {
        Q_D(Timeline);
        Q_ASSERT(loopStart >= 0);
        d->pModel->strategy->setEntityProperty(d->handle, ModelStrategy::P_LoopStart, loopStart);
    }

    int Timeline::loopLength() const {
        Q_D(const Timeline);
        return d->loopLength;
    }

    void Timeline::setLoopLength(int loopLength) {
        Q_D(Timeline);
        Q_ASSERT(loopLength > 0);
        d->pModel->strategy->setEntityProperty(d->handle, ModelStrategy::P_LoopLength, loopLength);
    }

    QDspx::Timeline Timeline::toQDspx() const {
        return {
            labels()->toQDspx(),
            tempos()->toQDspx(),
            timeSignatures()->toQDspx(),
        };
    }

    void Timeline::fromQDspx(const QDspx::Timeline &timeline) {
        Q_D(Timeline);
        labels()->fromQDspx(timeline.labels);
        tempos()->fromQDspx(timeline.tempos);
        timeSignatures()->fromQDspx(timeline.timeSignatures);
    }

}

#include "moc_Timeline.cpp"
