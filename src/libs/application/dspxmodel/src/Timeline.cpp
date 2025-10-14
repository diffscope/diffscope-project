#include "Timeline.h"

#include <dspxmodel/private/Model_p.h>

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

}