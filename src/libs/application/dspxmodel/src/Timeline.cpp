#include "Timeline.h"

#include <opendspx/timeline.h>

#include <dspxmodel/private/Model_p.h>
#include <dspxmodel/LabelSequence.h>
#include <dspxmodel/TempoSequence.h>
#include <dspxmodel/TimeSignatureSequence.h>

namespace dspx {

    class TimelinePrivate {
        Q_DECLARE_PUBLIC(Timeline)
    public:
        Timeline *q_ptr;
        ModelPrivate *pModel;
    };

    Timeline::Timeline(Model *model) : QObject(model), d_ptr(new TimelinePrivate) {
        Q_D(Timeline);
        d->q_ptr = this;
        d->pModel = ModelPrivate::get(model);
    }

    Timeline::~Timeline() = default;

    LabelSequence *Timeline::labels() const {
        Q_D(const Timeline);
        return d->pModel->labels;
    }

    TempoSequence *Timeline::tempos() const {
        Q_D(const Timeline);
        return d->pModel->tempos;
    }

    TimeSignatureSequence *Timeline::timeSignatures() const {
        Q_D(const Timeline);
        return d->pModel->timeSignatures;
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