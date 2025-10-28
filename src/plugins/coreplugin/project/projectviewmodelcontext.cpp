#include "projectviewmodelcontext.h"
#include "projectviewmodelcontext_p.h"

#include <QBindable>

#include <ScopicFlowCore/PlaybackViewModel.h>

#include <coreplugin/projecttimeline.h>

namespace Core {

    static void bindProjectTimeLineToViewModel(ProjectTimeline *projectTimeline, sflow::PlaybackViewModel *playbackViewModel) {
        QObject::connect(projectTimeline, &ProjectTimeline::positionChanged, playbackViewModel, [=] {
            playbackViewModel->setPrimaryPosition(projectTimeline->position());
        });
        QObject::connect(projectTimeline, &ProjectTimeline::lastPositionChanged, playbackViewModel, [=] {
            playbackViewModel->setSecondaryPosition(projectTimeline->lastPosition());
        });
        QObject::connect(playbackViewModel, &sflow::PlaybackViewModel::primaryPositionChanged, projectTimeline, [=] {
            projectTimeline->setPosition(playbackViewModel->primaryPosition());
        });
        QObject::connect(playbackViewModel, &sflow::PlaybackViewModel::secondaryPositionChanged, projectTimeline, [=] {
            projectTimeline->setLastPosition(playbackViewModel->secondaryPosition());
        });
        playbackViewModel->setPrimaryPosition(projectTimeline->position());
        playbackViewModel->setSecondaryPosition(projectTimeline->lastPosition());
    }

    ProjectViewModelContext::ProjectViewModelContext(ProjectTimeline *projectTimeline, QObject *parent) : QObject(parent), d_ptr(new ProjectViewModelContextPrivate) {
        Q_D(ProjectViewModelContext);
        d->q_ptr = this;
        d->playbackViewModel = new sflow::PlaybackViewModel(this);
        bindProjectTimeLineToViewModel(projectTimeline, d->playbackViewModel);
    }

    ProjectViewModelContext::~ProjectViewModelContext() = default;

    sflow::PlaybackViewModel *ProjectViewModelContext::playbackViewModel() const {
        Q_D(const ProjectViewModelContext);
        return d->playbackViewModel;
    }

}

#include "moc_projectviewmodelcontext.cpp"
