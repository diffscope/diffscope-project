#include "ProjectViewModelContext.h"
#include "ProjectViewModelContext_p.h"

#include <ScopicFlowCore/PlaybackViewModel.h>
#include <ScopicFlowCore/PointSequenceViewModel.h>
#include <ScopicFlowCore/LabelViewModel.h>
#include <ScopicFlowCore/LabelSequenceInteractionController.h>
#include <ScopicFlowCore/TimelineInteractionController.h>

#include <dspxmodel/Tempo.h>
#include <dspxmodel/Label.h>

#include <coreplugin/ProjectTimeline.h>
#include <coreplugin/ProjectWindowInterface.h>

#include <visualeditor/private/TempoSelectionController_p.h>
#include <visualeditor/private/LabelSelectionController_p.h>
#include <visualeditor/private/PlaybackViewModelContextData_p.h>
#include <visualeditor/private/TempoViewModelContextData_p.h>
#include <visualeditor/private/LabelViewModelContextData_p.h>

#include <memory>

namespace VisualEditor {

    ProjectViewModelContextAttachedType::ProjectViewModelContextAttachedType(QObject *parent) : QObject(parent) {
    }

    ProjectViewModelContextAttachedType::~ProjectViewModelContextAttachedType() = default;

    ProjectViewModelContext *ProjectViewModelContextAttachedType::context() const {
        auto windowHandle = qobject_cast<Core::ProjectWindowInterface *>(parent());
        if (!windowHandle) {
            return nullptr;
        }
        return ProjectViewModelContext::of(windowHandle);
    }

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

        d->labelData = std::make_unique<LabelViewModelContextData>();
        d->labelData->q_ptr = this;
        d->labelData->init();
        d->labelData->bindLabelSequenceViewModel();
    }

    ProjectViewModelContext::~ProjectViewModelContext() = default;

    ProjectViewModelContext *ProjectViewModelContext::of(const Core::ProjectWindowInterface *windowHandle) {
        return qobject_cast<ProjectViewModelContext *>(windowHandle->getFirstObject(staticMetaObject.className()));
    }

    ProjectViewModelContextAttachedType *ProjectViewModelContext::qmlAttachedProperties(QObject *object) {
        return new ProjectViewModelContextAttachedType(object);
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

    sflow::PointSequenceViewModel *ProjectViewModelContext::labelSequenceViewModel() const {
        Q_D(const ProjectViewModelContext);
        return d->labelData->labelSequenceViewModel;
    }

    sflow::SelectionController *ProjectViewModelContext::tempoSelectionController() const {
        Q_D(const ProjectViewModelContext);
        return d->tempoData->tempoSelectionController;
    }

    sflow::SelectionController *ProjectViewModelContext::labelSelectionController() const {
        Q_D(const ProjectViewModelContext);
        return d->labelData->labelSelectionController;
    }

    sflow::TimelineInteractionController *ProjectViewModelContext::createAndBindTimelineInteractionController(QObject *parent) {
        Q_D(ProjectViewModelContext);
        return d->playbackData->createController(parent);
    }

    sflow::LabelSequenceInteractionController *ProjectViewModelContext::createAndBindLabelSequenceInteractionControllerOfTempo(QObject *parent) {
        Q_D(ProjectViewModelContext);
        return d->tempoData->createController(parent);
    }

    sflow::LabelSequenceInteractionController *ProjectViewModelContext::createAndBindLabelSequenceInteractionControllerOfLabel(QObject *parent) {
        Q_D(ProjectViewModelContext);
        return d->labelData->createController(parent);
    }

    dspx::Tempo *ProjectViewModelContext::getTempoDocumentItemFromViewItem(sflow::LabelViewModel *viewItem) const {
        Q_D(const ProjectViewModelContext);
        return d->tempoData->tempoDocumentItemMap.value(viewItem);
    }

    sflow::LabelViewModel *ProjectViewModelContext::getTempoViewItemFromDocumentItem(dspx::Tempo *item) const {
        Q_D(const ProjectViewModelContext);
        return d->tempoData->tempoViewItemMap.value(item);
    }

    dspx::Label *ProjectViewModelContext::getLabelDocumentItemFromViewItem(sflow::LabelViewModel *viewItem) const {
        Q_D(const ProjectViewModelContext);
        return d->labelData->labelDocumentItemMap.value(viewItem);
    }

    sflow::LabelViewModel *ProjectViewModelContext::getLabelViewItemFromDocumentItem(dspx::Label *item) const {
        Q_D(const ProjectViewModelContext);
        return d->labelData->labelViewItemMap.value(item);
    }

}

#include "moc_ProjectViewModelContext.cpp"
