#include "ProjectViewModelContext.h"
#include "ProjectViewModelContext_p.h"

#include <QBindable>

#include <ScopicFlowCore/PlaybackViewModel.h>

#include <coreplugin/ProjectTimeline.h>
#include <coreplugin/ProjectWindowInterface.h>

namespace VisualEditor {

    static void bindProjectTimeLineToViewModel(Core::ProjectTimeline *projectTimeline, sflow::PlaybackViewModel *playbackViewModel) {
        QObject::connect(projectTimeline, &Core::ProjectTimeline::positionChanged, playbackViewModel, [=] {
            playbackViewModel->setPrimaryPosition(projectTimeline->position());
        });
        QObject::connect(projectTimeline, &Core::ProjectTimeline::lastPositionChanged, playbackViewModel, [=] {
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

    ProjectViewModelContext::ProjectViewModelContext(Core::ProjectWindowInterface *windowHandle) : QObject(windowHandle), d_ptr(new ProjectViewModelContextPrivate) {
        Q_D(ProjectViewModelContext);
        Q_ASSERT(windowHandle->getObjects(staticMetaObject.className()).isEmpty());
        windowHandle->addObject(staticMetaObject.className(), this);
        d->q_ptr = this;
        d->windowHandle = windowHandle;
        d->playbackViewModel = new sflow::PlaybackViewModel(this);
        bindProjectTimeLineToViewModel(windowHandle->projectTimeline(), d->playbackViewModel);
    }

    ProjectViewModelContext::~ProjectViewModelContext() = default;

    ProjectViewModelContext *ProjectViewModelContext::of(const Core::ProjectWindowInterface *windowHandle) {
        return qobject_cast<ProjectViewModelContext *>(windowHandle->getFirstObject(staticMetaObject.className()));
    }

    sflow::PlaybackViewModel *ProjectViewModelContext::playbackViewModel() const {
        Q_D(const ProjectViewModelContext);
        return d->playbackViewModel;
    }

}

#include "moc_ProjectViewModelContext.cpp"
