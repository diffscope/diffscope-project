#include "PlaybackViewModelContextData_p.h"

#include <QLoggingCategory>

#include <ScopicFlowCore/PlaybackViewModel.h>

#include <coreplugin/ProjectTimeline.h>
#include <coreplugin/ProjectWindowInterface.h>

#include <visualeditor/ProjectViewModelContext.h>

namespace VisualEditor {

    Q_STATIC_LOGGING_CATEGORY(lcPlaybackViewModelContextData, "diffscope.visualeditor.playbackviewmodelcontextdata")

    void PlaybackViewModelContextData::init() {
        Q_Q(ProjectViewModelContext);
        windowHandle = q->windowHandle();
        playbackViewModel = new sflow::PlaybackViewModel(q);
    }

    void PlaybackViewModelContextData::bindPlaybackViewModel() {
        auto projectTimeline = windowHandle->projectTimeline();
        QObject::connect(projectTimeline, &Core::ProjectTimeline::positionChanged, playbackViewModel, [=] {
            if (playbackViewModel->primaryPosition() == projectTimeline->position())
                return;
            qCDebug(lcPlaybackViewModelContextData) << "Project timeline position updated" << projectTimeline->position();
            playbackViewModel->setPrimaryPosition(projectTimeline->position());
        });
        QObject::connect(projectTimeline, &Core::ProjectTimeline::lastPositionChanged, playbackViewModel, [=] {
            if (playbackViewModel->secondaryPosition() == projectTimeline->lastPosition())
                return;
            qCDebug(lcPlaybackViewModelContextData) << "Project timeline last position updated" << projectTimeline->lastPosition();
            playbackViewModel->setSecondaryPosition(projectTimeline->lastPosition());
        });
        QObject::connect(playbackViewModel, &sflow::PlaybackViewModel::primaryPositionChanged, projectTimeline, [=] {
            if (projectTimeline->position() == playbackViewModel->primaryPosition())
                return;
            qCDebug(lcPlaybackViewModelContextData) << "Playback view model primary position updated" << playbackViewModel->primaryPosition();
            projectTimeline->setPosition(playbackViewModel->primaryPosition());
        });
        QObject::connect(playbackViewModel, &sflow::PlaybackViewModel::secondaryPositionChanged, projectTimeline, [=] {
            if (projectTimeline->lastPosition() == playbackViewModel->secondaryPosition())
                return;
            qCDebug(lcPlaybackViewModelContextData) << "Playback view model secondary position updated" << playbackViewModel->secondaryPosition();
            projectTimeline->setLastPosition(playbackViewModel->secondaryPosition());
        });
        playbackViewModel->setPrimaryPosition(projectTimeline->position());
        playbackViewModel->setSecondaryPosition(projectTimeline->lastPosition());
    }

}
