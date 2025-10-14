#include "Timeline.h"

#include <opendspx/qdspxmodel.h>

#include <dspxmodel/private/Model_p.h>
#include <dspxmodel/LabelSequence.h>

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

    QDspx::Timeline Timeline::toQDspx() const {
        return {
            .labels = labels()->toQDspx(),
            // TODO tempos timeSignatures
        };
    }

    void Timeline::fromQDspx(const QDspx::Timeline &timeline) {
        Q_D(Timeline);
        labels()->fromQDspx(timeline.labels);
        // TODO tempos timeSignatures
    }

}

#include "moc_Timeline.cpp"