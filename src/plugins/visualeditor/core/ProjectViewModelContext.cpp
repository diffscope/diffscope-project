#include "ProjectViewModelContext.h"
#include "ProjectViewModelContext_p.h"
#include "PlaybackViewModelContextData_p.h"
#include "TempoViewModelContextData_p.h"

#include <ScopicFlowCore/PlaybackViewModel.h>
#include <ScopicFlowCore/PointSequenceViewModel.h>
#include <ScopicFlowCore/LabelViewModel.h>
#include <ScopicFlowCore/LabelSequenceInteractionController.h>
#include <ScopicFlowCore/SelectionController.h>

#include <dspxmodel/Tempo.h>

#include <coreplugin/ProjectTimeline.h>
#include <coreplugin/ProjectWindowInterface.h>

#include <memory>

namespace VisualEditor {

    ProjectViewModelContext::ProjectViewModelContext(Core::ProjectWindowInterface *windowHandle) : QObject(windowHandle), d_ptr(new ProjectViewModelContextPrivate) {
        Q_D(ProjectViewModelContext);
        Q_ASSERT(windowHandle->getObjects(staticMetaObject.className()).isEmpty());
        windowHandle->addObject(staticMetaObject.className(), this);
        d->q_ptr = this;
        d->windowHandle = windowHandle;

        d->playbackData = std::make_unique<PlaybackViewModelContextData>();
        d->playbackData->q_ptr = this;
        d->playbackData->init();
        d->playbackData->bindPlaybackViewModel();

        d->tempoData = std::make_unique<TempoViewModelContextData>();
        d->tempoData->q_ptr = this;
        d->tempoData->init();
        d->tempoData->bindTempoSequenceViewModel();
    }

    ProjectViewModelContext::~ProjectViewModelContext() = default;

    ProjectViewModelContext *ProjectViewModelContext::of(const Core::ProjectWindowInterface *windowHandle) {
        return qobject_cast<ProjectViewModelContext *>(windowHandle->getFirstObject(staticMetaObject.className()));
    }

    Core::ProjectWindowInterface *ProjectViewModelContext::windowHandle() const {
        Q_D(const ProjectViewModelContext);
        return d->windowHandle;
    }

    sflow::PlaybackViewModel *ProjectViewModelContext::playbackViewModel() const {
        Q_D(const ProjectViewModelContext);
        return d->playbackData->playbackViewModel;
    }

    sflow::PointSequenceViewModel *ProjectViewModelContext::tempoSequenceViewModel() const {
        Q_D(const ProjectViewModelContext);
        return d->tempoData->tempoSequenceViewModel;
    }

    sflow::SelectionController *ProjectViewModelContext::tempoSelectionController() const {
        Q_D(const ProjectViewModelContext);
        return d->tempoData->tempoSelectionController;
    }

    sflow::LabelSequenceInteractionController *ProjectViewModelContext::createAndBindLabelSequenceInteractionControllerOfTempo(QObject *parent) {
        Q_D(ProjectViewModelContext);
        return d->tempoData->createController(parent);
    }

    dspx::Tempo *ProjectViewModelContext::getTempoDocumentItemFromViewItem(sflow::LabelViewModel *viewItem) const {
        Q_D(const ProjectViewModelContext);
        return d->tempoData->tempoDocumentItemMap.value(viewItem);
    }

    sflow::LabelViewModel *ProjectViewModelContext::getTempoViewItemFromDocumentItem(dspx::Tempo *item) const {
        Q_D(const ProjectViewModelContext);
        return d->tempoData->tempoViewItemMap.value(item);
    }

}

#include "moc_ProjectViewModelContext.cpp"
