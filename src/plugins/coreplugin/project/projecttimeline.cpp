#include "projecttimeline.h"
#include "projecttimeline_p.h"

#include <SVSCraftCore/MusicTimeline.h>

namespace Core {

    ProjectTimeline::ProjectTimeline(QObject *parent) : QObject(parent), d_ptr(new ProjectTimelinePrivate) {
        Q_D(ProjectTimeline);
        d->q_ptr = this;
        d->musicTimeline = new SVS::MusicTimeline(this);
    }

    ProjectTimeline::~ProjectTimeline() = default;

    SVS::MusicTimeline* ProjectTimeline::musicTimeline() const {
        Q_D(const ProjectTimeline);
        return d->musicTimeline;
    }

    int ProjectTimeline::position() const {
        Q_D(const ProjectTimeline);
        return d->position;
    }
    void ProjectTimeline::setPosition(int position) {
        Q_D(ProjectTimeline);
        if (d->position != position) {
            d->position = position;
            if (position >= d->rangeHint) {
                d->rangeHint = position + 1;
                Q_EMIT rangeHintChanged(d->rangeHint);
            }
            Q_EMIT positionChanged(position);
        }
    }

    int ProjectTimeline::lastPosition() const {
        Q_D(const ProjectTimeline);
        return d->lastPosition;
    }
    void ProjectTimeline::setLastPosition(int lastPosition) {
        Q_D(ProjectTimeline);
        if (d->lastPosition != lastPosition) {
            d->lastPosition = lastPosition;
            if (lastPosition >= d->rangeHint) {
                d->rangeHint = lastPosition + 1;
                Q_EMIT rangeHintChanged(d->rangeHint);
            }
            Q_EMIT lastPositionChanged(lastPosition);
        }
    }

    int ProjectTimeline::rangeHint() const {
        Q_D(const ProjectTimeline);
        return d->rangeHint;
    }
    void ProjectTimeline::setRangeHint(int rangeHint) {
        Q_D(ProjectTimeline);
        if (d->rangeHint != rangeHint) {
            d->rangeHint = rangeHint;
            Q_EMIT rangeHintChanged(rangeHint);
        }
    }
    void ProjectTimeline::goTo(int position) {
        setPosition(position);
        setLastPosition(position);
    }

}

#include "moc_projecttimeline.cpp"