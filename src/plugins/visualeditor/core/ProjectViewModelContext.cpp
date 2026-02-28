#include "ProjectViewModelContext.h"
#include "ProjectViewModelContext_p.h"

#include <memory>

#include <ScopicFlowCore/ClipViewModel.h>
#include <ScopicFlowCore/PlaybackViewModel.h>
#include <ScopicFlowCore/ListViewModel.h>
#include <ScopicFlowCore/NoteViewModel.h>
#include <ScopicFlowCore/PointSequenceViewModel.h>
#include <ScopicFlowCore/LabelViewModel.h>
#include <ScopicFlowCore/RangeSequenceViewModel.h>
#include <ScopicFlowCore/TrackViewModel.h>
#include <ScopicFlowCore/LabelSequenceInteractionController.h>
#include <ScopicFlowCore/TimelineInteractionController.h>
#include <ScopicFlowCore/TrackListInteractionController.h>
#include <ScopicFlowCore/ClipPaneInteractionController.h>
#include <ScopicFlowCore/NoteEditLayerInteractionController.h>

#include <dspxmodel/Clip.h>
#include <dspxmodel/KeySignature.h>
#include <dspxmodel/Note.h>
#include <dspxmodel/SingingClip.h>
#include <dspxmodel/Tempo.h>
#include <dspxmodel/Label.h>
#include <dspxmodel/Track.h>

#include <coreplugin/ProjectTimeline.h>
#include <coreplugin/ProjectWindowInterface.h>

#include <visualeditor/private/TempoSelectionController_p.h>
#include <visualeditor/private/KeySignatureSelectionController_p.h>
#include <visualeditor/private/LabelSelectionController_p.h>
#include <visualeditor/private/ClipViewModelContextData_p.h>
#include <visualeditor/private/TrackSelectionController_p.h>
#include <visualeditor/private/PlaybackViewModelContextData_p.h>
#include <visualeditor/private/TempoViewModelContextData_p.h>
#include <visualeditor/private/KeySignatureViewModelContextData_p.h>
#include <visualeditor/private/LabelViewModelContextData_p.h>
#include <visualeditor/private/ClipSelectionController_p.h>
#include <visualeditor/private/TrackViewModelContextData_p.h>
#include <visualeditor/private/MasterTrackViewModelContextData_p.h>
#include <visualeditor/private/NoteViewModelContextData_p.h>
#include <visualeditor/private/NoteSelectionController_p.h>

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

        d->keySignatureData = std::make_unique<KeySignatureViewModelContextData>();
        d->keySignatureData->q_ptr = this;
        d->keySignatureData->init();
        d->keySignatureData->bindKeySignatureSequenceViewModel();

        d->labelData = std::make_unique<LabelViewModelContextData>();
        d->labelData->q_ptr = this;
        d->labelData->init();
        d->labelData->bindLabelSequenceViewModel();

        d->clipData = std::make_unique<ClipViewModelContextData>();
        d->clipData->q_ptr = this;
        d->clipData->init();
        d->clipData->bindClipSequences();

        d->noteData = std::make_unique<NoteViewModelContextData>();
        d->noteData->q_ptr = this;
        d->noteData->init();
        d->noteData->bindTrackSequences();

        d->trackData = std::make_unique<TrackViewModelContextData>();
        d->trackData->q_ptr = this;
        d->trackData->init();
        d->trackData->bindTrackListViewModel();

        d->masterTrackData = std::make_unique<MasterTrackViewModelContextData>();
        d->masterTrackData->q_ptr = this;
        d->masterTrackData->init();
        d->masterTrackData->bindMasterTrackViewModel();
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

    sflow::PointSequenceViewModel *ProjectViewModelContext::keySignatureSequenceViewModel() const {
        Q_D(const ProjectViewModelContext);
        return d->keySignatureData->keySignatureSequenceViewModel;
    }

    sflow::PointSequenceViewModel *ProjectViewModelContext::labelSequenceViewModel() const {
        Q_D(const ProjectViewModelContext);
        return d->labelData->labelSequenceViewModel;
    }

    sflow::RangeSequenceViewModel *ProjectViewModelContext::clipSequenceViewModel() const {
        Q_D(const ProjectViewModelContext);
        return d->clipData->clipSequenceViewModel;
    }

    sflow::ListViewModel *ProjectViewModelContext::trackListViewModel() const {
        Q_D(const ProjectViewModelContext);
        return d->trackData->trackListViewModel;
    }

    sflow::ListViewModel *ProjectViewModelContext::masterTrackListViewModel() const {
        Q_D(const ProjectViewModelContext);
        return d->masterTrackData->masterTrackListViewModel;
    }

    sflow::SelectionController *ProjectViewModelContext::tempoSelectionController() const {
        Q_D(const ProjectViewModelContext);
        return d->tempoData->tempoSelectionController;
    }

    sflow::SelectionController *ProjectViewModelContext::keySignatureSelectionController() const {
        Q_D(const ProjectViewModelContext);
        return d->keySignatureData->keySignatureSelectionController;
    }

    sflow::SelectionController *ProjectViewModelContext::labelSelectionController() const {
        Q_D(const ProjectViewModelContext);
        return d->labelData->labelSelectionController;
    }

    sflow::SelectionController *ProjectViewModelContext::clipSelectionController() const {
        Q_D(const ProjectViewModelContext);
        return d->clipData->clipSelectionController;
    }

    sflow::SelectionController *ProjectViewModelContext::noteSelectionController() const {
        Q_D(const ProjectViewModelContext);
        return d->noteData->noteSelectionController;
    }

    sflow::SelectionController *ProjectViewModelContext::trackSelectionController() const {
        Q_D(const ProjectViewModelContext);
        return d->trackData->trackSelectionController;
    }

    sflow::TimelineInteractionController *ProjectViewModelContext::createAndBindTimelineInteractionController(QObject *parent) {
        Q_D(ProjectViewModelContext);
        return d->playbackData->createController(parent);
    }

    sflow::LabelSequenceInteractionController *ProjectViewModelContext::createAndBindLabelSequenceInteractionControllerOfTempo(QObject *parent) {
        Q_D(ProjectViewModelContext);
        return d->tempoData->createController(parent);
    }

    sflow::LabelSequenceInteractionController *ProjectViewModelContext::createAndBindLabelSequenceInteractionControllerOfKeySignature(QObject *parent) {
        Q_D(ProjectViewModelContext);
        return d->keySignatureData->createController(parent);
    }

    sflow::LabelSequenceInteractionController *ProjectViewModelContext::createAndBindLabelSequenceInteractionControllerOfLabel(QObject *parent) {
        Q_D(ProjectViewModelContext);
        return d->labelData->createController(parent);
    }

    sflow::ClipPaneInteractionController *ProjectViewModelContext::createAndBindClipPaneInteractionController(QObject *parent) {
        Q_D(ProjectViewModelContext);
        return d->clipData->createController(parent);
    }

    sflow::NoteEditLayerInteractionController *ProjectViewModelContext::createAndBindNoteEditLayerInteractionController(QObject *parent) {
        Q_D(ProjectViewModelContext);
        return d->noteData->createController(parent);
    }

    sflow::TrackListInteractionController *ProjectViewModelContext::createAndBindTrackListInteractionController(QObject *parent) {
        Q_D(ProjectViewModelContext);
        return d->trackData->createController(parent);
    }

    sflow::TrackListInteractionController *ProjectViewModelContext::createAndBindTrackListInteractionControllerOfMaster(QObject *parent) {
        Q_D(ProjectViewModelContext);
        return d->masterTrackData->createController(parent);
    }

    dspx::Clip *ProjectViewModelContext::getClipDocumentItemFromViewItem(sflow::ClipViewModel *viewItem) const {
        Q_D(const ProjectViewModelContext);
        return d->clipData->clipDocumentItemMap.value(viewItem);
    }

    sflow::ClipViewModel *ProjectViewModelContext::getClipViewItemFromDocumentItem(dspx::Clip *item) const {
        Q_D(const ProjectViewModelContext);
        return d->clipData->clipViewItemMap.value(item);
    }

    dspx::Tempo *ProjectViewModelContext::getTempoDocumentItemFromViewItem(sflow::LabelViewModel *viewItem) const {
        Q_D(const ProjectViewModelContext);
        return d->tempoData->tempoDocumentItemMap.value(viewItem);
    }

    sflow::LabelViewModel *ProjectViewModelContext::getTempoViewItemFromDocumentItem(dspx::Tempo *item) const {
        Q_D(const ProjectViewModelContext);
        return d->tempoData->tempoViewItemMap.value(item);
    }

    dspx::KeySignature *ProjectViewModelContext::getKeySignatureDocumentItemFromViewItem(sflow::LabelViewModel *viewItem) const {
        Q_D(const ProjectViewModelContext);
        return d->keySignatureData->keySignatureDocumentItemMap.value(viewItem);
    }

    sflow::LabelViewModel *ProjectViewModelContext::getKeySignatureViewItemFromDocumentItem(dspx::KeySignature *item) const {
        Q_D(const ProjectViewModelContext);
        return d->keySignatureData->keySignatureViewItemMap.value(item);
    }

    dspx::Label *ProjectViewModelContext::getLabelDocumentItemFromViewItem(sflow::LabelViewModel *viewItem) const {
        Q_D(const ProjectViewModelContext);
        return d->labelData->labelDocumentItemMap.value(viewItem);
    }

    sflow::LabelViewModel *ProjectViewModelContext::getLabelViewItemFromDocumentItem(dspx::Label *item) const {
        Q_D(const ProjectViewModelContext);
        return d->labelData->labelViewItemMap.value(item);
    }

    dspx::Note *ProjectViewModelContext::getNoteDocumentItemFromViewItem(sflow::NoteViewModel *viewItem) const {
        Q_D(const ProjectViewModelContext);
        return d->noteData->noteDocumentItemMap.value(viewItem);
    }

    sflow::NoteViewModel *ProjectViewModelContext::getNoteViewItemFromDocumentItem(dspx::Note *item) const {
        Q_D(const ProjectViewModelContext);
        return d->noteData->noteViewItemMap.value(item);
    }

    dspx::Track *ProjectViewModelContext::getTrackDocumentItemFromViewItem(sflow::TrackViewModel *viewItem) const {
        Q_D(const ProjectViewModelContext);
        return d->trackData->trackDocumentItemMap.value(viewItem);
    }

    sflow::TrackViewModel *ProjectViewModelContext::getTrackViewItemFromDocumentItem(dspx::Track *item) const {
        Q_D(const ProjectViewModelContext);
        return d->trackData->trackViewItemMap.value(item);
    }

    sflow::RangeSequenceViewModel *ProjectViewModelContext::getSingingClipPerTrackSequenceViewModel(dspx::Track *track) const {
        Q_D(const ProjectViewModelContext);
        return d->noteData->singingClipPerTrackSequenceViewModelMap.value(track);
    }

    sflow::RangeSequenceViewModel *ProjectViewModelContext::getNoteSequenceViewModel(dspx::SingingClip *clip) const {
        Q_D(const ProjectViewModelContext);
        return d->noteData->noteSequenceViewModelMap.value(clip);
    }

}

#include "moc_ProjectViewModelContext.cpp"
