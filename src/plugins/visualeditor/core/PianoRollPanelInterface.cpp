#include "PianoRollPanelInterface.h"
#include "PianoRollPanelInterface_p.h"

#include <cmath>

#include <QLoggingCategory>
#include <QQmlComponent>
#include <QQuickItem>
#include <QSortFilterProxyModel>
#include <QVariant>

#include <CoreApi/runtimeinterface.h>

#include <ScopicFlowCore/ClavierInteractionController.h>
#include <ScopicFlowCore/ClavierViewModel.h>
#include <ScopicFlowCore/LabelSequenceInteractionController.h>
#include <ScopicFlowCore/NoteEditLayerInteractionController.h>
#include <ScopicFlowCore/ScrollBehaviorViewModel.h>
#include <ScopicFlowCore/TimeLayoutViewModel.h>
#include <ScopicFlowCore/TimeManipulator.h>
#include <ScopicFlowCore/TimeViewModel.h>
#include <ScopicFlowCore/TimelineInteractionController.h>

#include <dspxmodel/ClipSelectionModel.h>
#include <dspxmodel/Global.h>
#include <dspxmodel/Model.h>
#include <dspxmodel/Note.h>
#include <dspxmodel/NoteSelectionModel.h>
#include <dspxmodel/NoteSequence.h>
#include <dspxmodel/SelectionModel.h>
#include <dspxmodel/SingingClip.h>
#include <dspxmodel/TrackList.h>
#include <dspxmodel/Timeline.h>

#include <coreplugin/CoreInterface.h>
#include <coreplugin/DefaultLyricManager.h>
#include <coreplugin/DspxDocument.h>
#include <coreplugin/ProjectDocumentContext.h>
#include <coreplugin/ProjectTimeline.h>
#include <coreplugin/ProjectWindowInterface.h>
#include <coreplugin/internal/KeySignatureAtSpecifiedPositionHelper.h>

#include <transactional/TransactionController.h>
#include <visualeditor/AutoPageScrollingManipulator.h>
#include <visualeditor/PositionAlignmentManipulator.h>
#include <visualeditor/ProjectViewModelContext.h>
#include <visualeditor/internal/EditorPreference.h>
#include <visualeditor/internal/PianoRollAddOn.h>
#include <visualeditor/internal/SingingClipListModel.h>
#include <visualeditor/internal/TrackOverlaySelectorModel.h>

namespace VisualEditor {

    Q_STATIC_LOGGING_CATEGORY(lcPianoRollPanelInterface, "diffscope.visualeditor.pianorollpanelinterface")

    void PianoRollPanelInterfacePrivate::bindTimeViewModel() const {
        auto projectTimeline = windowHandle->projectTimeline();
        timeViewModel->setTimeline(projectTimeline->musicTimeline());
        timeViewModel->setEnd(projectTimeline->rangeHint());
        QObject::connect(projectTimeline, &Core::ProjectTimeline::rangeHintChanged, timeViewModel, [=, this] {
            timeViewModel->setEnd(projectTimeline->rangeHint());
        });
        QObject::connect(timeViewModel, &sflow::TimeViewModel::endChanged, projectTimeline, [=, this] {
            projectTimeline->setRangeHint(static_cast<int>(std::ceil(timeViewModel->end())));
        });
    }

    void PianoRollPanelInterfacePrivate::bindTimeLayoutViewModel() const {
    }

    void PianoRollPanelInterfacePrivate::bindTimelineInteractionController() const {
    }

    static Qt::KeyboardModifier getModifier(Internal::EditorPreference::ScrollModifier modifier) {
        switch (modifier) {
            case Internal::EditorPreference::SM_Control:
                return Qt::ControlModifier;
            case Internal::EditorPreference::SM_Alt:
                return Qt::AltModifier;
            case Internal::EditorPreference::SM_Shift:
                return Qt::ShiftModifier;
        }
        Q_UNREACHABLE();
    }

    static sflow::ScrollBehaviorViewModel::ScrollTypes getScrollTypes(PianoRollPanelInterface::Tool tool) {
        if (tool == PianoRollPanelInterface::HandTool) {
            return sflow::ScrollBehaviorViewModel::Wheel | sflow::ScrollBehaviorViewModel::Pinch | sflow::ScrollBehaviorViewModel::MiddleButton | sflow::ScrollBehaviorViewModel::LeftButton;
        }
        return sflow::ScrollBehaviorViewModel::Wheel | sflow::ScrollBehaviorViewModel::Pinch | sflow::ScrollBehaviorViewModel::MiddleButton;
    }

    void PianoRollPanelInterfacePrivate::bindScrollBehaviorViewModel() const {
        Q_Q(const PianoRollPanelInterface);
        scrollBehaviorViewModel->setAlternateAxisModifier(getModifier(Internal::EditorPreference::alternateAxisModifier()));
        scrollBehaviorViewModel->setZoomModifier(getModifier(Internal::EditorPreference::zoomModifier()));
        scrollBehaviorViewModel->setPageModifier(getModifier(Internal::EditorPreference::pageModifier()));
        scrollBehaviorViewModel->setUsePageModifierAsAlternateAxisZoom(Internal::EditorPreference::usePageModifierAsAlternateAxisZoom());
        scrollBehaviorViewModel->setAutoScroll(Internal::EditorPreference::middleButtonAutoScroll());

        QObject::connect(Internal::EditorPreference::instance(), &Internal::EditorPreference::alternateAxisModifierChanged, scrollBehaviorViewModel, [=, this] {
            scrollBehaviorViewModel->setAlternateAxisModifier(getModifier(Internal::EditorPreference::alternateAxisModifier()));
        });
        QObject::connect(Internal::EditorPreference::instance(), &Internal::EditorPreference::zoomModifierChanged, scrollBehaviorViewModel, [=, this] {
            scrollBehaviorViewModel->setZoomModifier(getModifier(Internal::EditorPreference::zoomModifier()));
        });
        QObject::connect(Internal::EditorPreference::instance(), &Internal::EditorPreference::pageModifierChanged, scrollBehaviorViewModel, [=, this] {
            scrollBehaviorViewModel->setPageModifier(getModifier(Internal::EditorPreference::pageModifier()));
        });
        QObject::connect(Internal::EditorPreference::instance(), &Internal::EditorPreference::usePageModifierAsAlternateAxisZoomChanged, scrollBehaviorViewModel, [=, this] {
            scrollBehaviorViewModel->setUsePageModifierAsAlternateAxisZoom(Internal::EditorPreference::usePageModifierAsAlternateAxisZoom());
        });
        QObject::connect(Internal::EditorPreference::instance(), &Internal::EditorPreference::middleButtonAutoScrollChanged, scrollBehaviorViewModel, [=, this] {
            scrollBehaviorViewModel->setAutoScroll(Internal::EditorPreference::middleButtonAutoScroll());
        });

        scrollBehaviorViewModel->setScrollTypes(getScrollTypes(tool));
        QObject::connect(q, &PianoRollPanelInterface::toolChanged, scrollBehaviorViewModel, [=, this] {
            scrollBehaviorViewModel->setScrollTypes(getScrollTypes(tool));
        });
    }

    void PianoRollPanelInterfacePrivate::bindPositionAlignmentManipulator() const {
        Q_Q(const PianoRollPanelInterface);
        positionAlignmentManipulator->setAutoDurationPositionAlignment(Internal::EditorPreference::autoDurationPositionAlignment());
        QObject::connect(Internal::EditorPreference::instance(), &Internal::EditorPreference::autoDurationPositionAlignmentChanged, positionAlignmentManipulator, [=, this] {
            positionAlignmentManipulator->setAutoDurationPositionAlignment(Internal::EditorPreference::autoDurationPositionAlignment());
        });
        QObject::connect(q, &PianoRollPanelInterface::snapTemporarilyDisabledChanged, positionAlignmentManipulator, [=, this] {
            if (isSnapTemporarilyDisabled) {
                previousDuration = positionAlignmentManipulator->duration();
                previousPositionAlignment = timeLayoutViewModel->positionAlignment();
                positionAlignmentManipulator->setDuration(PositionAlignmentManipulator::Unset);
                timeLayoutViewModel->setDisplayPositionAlignment(previousPositionAlignment);
            } else {
                positionAlignmentManipulator->setDuration(previousDuration);
                timeLayoutViewModel->resetDisplayPositionAlignment();
            }
        });
    }

    void PianoRollPanelInterfacePrivate::bindControllersInteraction() const {
        Q_Q(const PianoRollPanelInterface);
        QObject::connect(q, &PianoRollPanelInterface::toolChanged, q, [=, this] {
            switch (tool) {
                case PianoRollPanelInterface::PointerTool: {
                    labelSequenceInteractionControllerOfLabel->setPrimaryItemInteraction(sflow::LabelSequenceInteractionController::Move);
                    labelSequenceInteractionControllerOfLabel->setSecondaryItemInteraction(sflow::LabelSequenceInteractionController::CopyAndMove);
                    labelSequenceInteractionControllerOfLabel->setPrimarySceneInteraction(sflow::LabelSequenceInteractionController::RubberBandSelect);
                    labelSequenceInteractionControllerOfLabel->setSecondarySceneInteraction(sflow::LabelSequenceInteractionController::RubberBandSelect);

                    labelSequenceInteractionControllerOfTempo->setPrimaryItemInteraction(sflow::LabelSequenceInteractionController::Move);
                    labelSequenceInteractionControllerOfTempo->setSecondaryItemInteraction(sflow::LabelSequenceInteractionController::Move);
                    labelSequenceInteractionControllerOfTempo->setPrimarySceneInteraction(sflow::LabelSequenceInteractionController::RubberBandSelect);
                    labelSequenceInteractionControllerOfTempo->setSecondarySceneInteraction(sflow::LabelSequenceInteractionController::RubberBandSelect);

                    labelSequenceInteractionControllerOfKeySignature->setPrimaryItemInteraction(sflow::LabelSequenceInteractionController::Move);
                    labelSequenceInteractionControllerOfKeySignature->setSecondaryItemInteraction(sflow::LabelSequenceInteractionController::Move);
                    labelSequenceInteractionControllerOfKeySignature->setPrimarySceneInteraction(sflow::LabelSequenceInteractionController::RubberBandSelect);
                    labelSequenceInteractionControllerOfKeySignature->setSecondarySceneInteraction(sflow::LabelSequenceInteractionController::RubberBandSelect);

                    noteEditLayerInteractionController->setPrimaryItemInteraction(sflow::NoteEditLayerInteractionController::Move);
                    noteEditLayerInteractionController->setSecondaryItemInteraction(sflow::NoteEditLayerInteractionController::CopyAndMove);
                    noteEditLayerInteractionController->setPrimarySceneInteraction(sflow::NoteEditLayerInteractionController::RubberBandSelect);
                    noteEditLayerInteractionController->setSecondarySceneInteraction(sflow::NoteEditLayerInteractionController::TimeRangeSelect);
                    break;
                }
                case PianoRollPanelInterface::PencilTool: {
                    labelSequenceInteractionControllerOfLabel->setPrimaryItemInteraction(sflow::LabelSequenceInteractionController::Move);
                    labelSequenceInteractionControllerOfLabel->setSecondaryItemInteraction(sflow::LabelSequenceInteractionController::CopyAndMove);
                    labelSequenceInteractionControllerOfLabel->setPrimarySceneInteraction(sflow::LabelSequenceInteractionController::RubberBandSelect);
                    labelSequenceInteractionControllerOfLabel->setSecondarySceneInteraction(sflow::LabelSequenceInteractionController::RubberBandSelect);

                    labelSequenceInteractionControllerOfTempo->setPrimaryItemInteraction(sflow::LabelSequenceInteractionController::Move);
                    labelSequenceInteractionControllerOfTempo->setSecondaryItemInteraction(sflow::LabelSequenceInteractionController::Move);
                    labelSequenceInteractionControllerOfTempo->setPrimarySceneInteraction(sflow::LabelSequenceInteractionController::RubberBandSelect);
                    labelSequenceInteractionControllerOfTempo->setSecondarySceneInteraction(sflow::LabelSequenceInteractionController::RubberBandSelect);

                    labelSequenceInteractionControllerOfKeySignature->setPrimaryItemInteraction(sflow::LabelSequenceInteractionController::Move);
                    labelSequenceInteractionControllerOfKeySignature->setSecondaryItemInteraction(sflow::LabelSequenceInteractionController::Move);
                    labelSequenceInteractionControllerOfKeySignature->setPrimarySceneInteraction(sflow::LabelSequenceInteractionController::RubberBandSelect);
                    labelSequenceInteractionControllerOfKeySignature->setSecondarySceneInteraction(sflow::LabelSequenceInteractionController::RubberBandSelect);

                    noteEditLayerInteractionController->setPrimaryItemInteraction(sflow::NoteEditLayerInteractionController::Move);
                    noteEditLayerInteractionController->setSecondaryItemInteraction(sflow::NoteEditLayerInteractionController::Draw);
                    noteEditLayerInteractionController->setPrimarySceneInteraction(sflow::NoteEditLayerInteractionController::Draw);
                    noteEditLayerInteractionController->setSecondarySceneInteraction(sflow::NoteEditLayerInteractionController::Draw);
                    break;
                }
                case PianoRollPanelInterface::ScissorTool: {
                    labelSequenceInteractionControllerOfLabel->setPrimaryItemInteraction(sflow::LabelSequenceInteractionController::Move);
                    labelSequenceInteractionControllerOfLabel->setSecondaryItemInteraction(sflow::LabelSequenceInteractionController::CopyAndMove);
                    labelSequenceInteractionControllerOfLabel->setPrimarySceneInteraction(sflow::LabelSequenceInteractionController::RubberBandSelect);
                    labelSequenceInteractionControllerOfLabel->setSecondarySceneInteraction(sflow::LabelSequenceInteractionController::RubberBandSelect);

                    labelSequenceInteractionControllerOfTempo->setPrimaryItemInteraction(sflow::LabelSequenceInteractionController::Move);
                    labelSequenceInteractionControllerOfTempo->setSecondaryItemInteraction(sflow::LabelSequenceInteractionController::Move);
                    labelSequenceInteractionControllerOfTempo->setPrimarySceneInteraction(sflow::LabelSequenceInteractionController::RubberBandSelect);
                    labelSequenceInteractionControllerOfTempo->setSecondarySceneInteraction(sflow::LabelSequenceInteractionController::RubberBandSelect);

                    labelSequenceInteractionControllerOfKeySignature->setPrimaryItemInteraction(sflow::LabelSequenceInteractionController::Move);
                    labelSequenceInteractionControllerOfKeySignature->setSecondaryItemInteraction(sflow::LabelSequenceInteractionController::Move);
                    labelSequenceInteractionControllerOfKeySignature->setPrimarySceneInteraction(sflow::LabelSequenceInteractionController::RubberBandSelect);
                    labelSequenceInteractionControllerOfKeySignature->setSecondarySceneInteraction(sflow::LabelSequenceInteractionController::RubberBandSelect);

                    noteEditLayerInteractionController->setPrimaryItemInteraction(sflow::NoteEditLayerInteractionController::Split);
                    noteEditLayerInteractionController->setSecondaryItemInteraction(sflow::NoteEditLayerInteractionController::Split);
                    noteEditLayerInteractionController->setPrimarySceneInteraction(sflow::NoteEditLayerInteractionController::RubberBandSelect);
                    noteEditLayerInteractionController->setSecondarySceneInteraction(sflow::NoteEditLayerInteractionController::TimeRangeSelect);
                    break;
                }
                case PianoRollPanelInterface::SelectTool: {
                    labelSequenceInteractionControllerOfLabel->setPrimaryItemInteraction(sflow::LabelSequenceInteractionController::RubberBandSelect);
                    labelSequenceInteractionControllerOfLabel->setSecondaryItemInteraction(sflow::LabelSequenceInteractionController::RubberBandSelect);
                    labelSequenceInteractionControllerOfLabel->setPrimarySceneInteraction(sflow::LabelSequenceInteractionController::RubberBandSelect);
                    labelSequenceInteractionControllerOfLabel->setSecondarySceneInteraction(sflow::LabelSequenceInteractionController::RubberBandSelect);

                    labelSequenceInteractionControllerOfTempo->setPrimaryItemInteraction(sflow::LabelSequenceInteractionController::RubberBandSelect);
                    labelSequenceInteractionControllerOfTempo->setSecondaryItemInteraction(sflow::LabelSequenceInteractionController::RubberBandSelect);
                    labelSequenceInteractionControllerOfTempo->setPrimarySceneInteraction(sflow::LabelSequenceInteractionController::RubberBandSelect);
                    labelSequenceInteractionControllerOfTempo->setSecondarySceneInteraction(sflow::LabelSequenceInteractionController::RubberBandSelect);

                    labelSequenceInteractionControllerOfKeySignature->setPrimaryItemInteraction(sflow::LabelSequenceInteractionController::RubberBandSelect);
                    labelSequenceInteractionControllerOfKeySignature->setSecondaryItemInteraction(sflow::LabelSequenceInteractionController::RubberBandSelect);
                    labelSequenceInteractionControllerOfKeySignature->setPrimarySceneInteraction(sflow::LabelSequenceInteractionController::RubberBandSelect);
                    labelSequenceInteractionControllerOfKeySignature->setSecondarySceneInteraction(sflow::LabelSequenceInteractionController::RubberBandSelect);

                    noteEditLayerInteractionController->setPrimaryItemInteraction(sflow::NoteEditLayerInteractionController::RubberBandSelect);
                    noteEditLayerInteractionController->setSecondaryItemInteraction(sflow::NoteEditLayerInteractionController::TimeRangeSelect);
                    noteEditLayerInteractionController->setPrimarySceneInteraction(sflow::NoteEditLayerInteractionController::RubberBandSelect);
                    noteEditLayerInteractionController->setSecondarySceneInteraction(sflow::NoteEditLayerInteractionController::TimeRangeSelect);
                    break;
                }
                case PianoRollPanelInterface::HandTool:
                    break;
            }
        });
    }

    void PianoRollPanelInterfacePrivate::bindClavierInteractionController() const {
        Q_Q(const PianoRollPanelInterface);
        auto applyStyle = [=, this] {
            auto simple = Internal::EditorPreference::pianoKeyboardUseSimpleStyle();
            clavierInteractionController->setDisplayStyle(simple ? sflow::ClavierInteractionController::Simple : sflow::ClavierInteractionController::Realistic);
        };
        applyStyle();
        QObject::connect(Internal::EditorPreference::instance(), &Internal::EditorPreference::pianoKeyboardUseSimpleStyleChanged, clavierInteractionController, applyStyle);

        auto applyLabelStrategy = [=, this] {
            auto policy = Internal::EditorPreference::pianoKeyboardLabelPolicy();
            switch (policy) {
                case Internal::EditorPreference::LP_All:
                    clavierInteractionController->setLabelStrategy(sflow::ClavierInteractionController::LabelAll);
                    break;
                case Internal::EditorPreference::LP_COnly:
                    clavierInteractionController->setLabelStrategy(sflow::ClavierInteractionController::LabelC);
                    break;
                case Internal::EditorPreference::LP_None:
                    clavierInteractionController->setLabelStrategy(sflow::ClavierInteractionController::LabelNone);
                    break;
            }
        };
        applyLabelStrategy();
        QObject::connect(Internal::EditorPreference::instance(), &Internal::EditorPreference::pianoKeyboardLabelPolicyChanged, clavierInteractionController, applyLabelStrategy);

        auto keySignatureAtSpecifiedPositionHelper = new Core::Internal::KeySignatureAtSpecifiedPositionHelper(const_cast<PianoRollPanelInterface *>(q));
        keySignatureAtSpecifiedPositionHelper->setKeySignatureSequence(windowHandle->projectDocumentContext()->document()->model()->timeline()->keySignatures());
        keySignatureAtSpecifiedPositionHelper->setPosition(windowHandle->projectTimeline()->position());
        clavierInteractionController->setAccidentalType(static_cast<sflow::ClavierInteractionController::AccidentalType>(keySignatureAtSpecifiedPositionHelper->accidentalType()));
        QObject::connect(windowHandle->projectTimeline(), &Core::ProjectTimeline::positionChanged, keySignatureAtSpecifiedPositionHelper, &Core::Internal::KeySignatureAtSpecifiedPositionHelper::setPosition);
        QObject::connect(keySignatureAtSpecifiedPositionHelper, &Core::Internal::KeySignatureAtSpecifiedPositionHelper::accidentalTypeChanged, clavierInteractionController, [=] {
            clavierInteractionController->setAccidentalType(static_cast<sflow::ClavierInteractionController::AccidentalType>(keySignatureAtSpecifiedPositionHelper->accidentalType()));
        });
    }

    void PianoRollPanelInterfacePrivate::bindNoteEditLayerInteractionController() const {
        Q_Q(const PianoRollPanelInterface);
        QObject::connect(noteEditLayerInteractionController, &sflow::NoteEditLayerInteractionController::doubleClicked, q, [=, this](QQuickItem *noteArea, int position, int key) {
            auto singingClip = q->editingClip();
            if (!singingClip)
                return;
            dspx::Note *newNote = nullptr;
            bool success = false;
            auto document = windowHandle->projectDocumentContext()->document();
            {
                sflow::TimeManipulator timeManipulator;
                timeManipulator.setTarget(noteArea);
                timeManipulator.setTimeViewModel(timeViewModel);
                timeManipulator.setTimeLayoutViewModel(timeLayoutViewModel);
                position = timeManipulator.alignPosition(position, sflow::ScopicFlow::AO_Visible);
            }
            document->transactionController()->beginScopedTransaction(VisualEditor::PianoRollPanelInterface::tr("Inserting note"), [=, &newNote, &success] {
                newNote = document->model()->createNote();
                newNote->setPos(position);
                newNote->setLength(implicitNoteLength);
                newNote->setKeyNum(key);
                newNote->setLyric(Core::CoreInterface::defaultLyricManager()->getDefaultLyricForSingingClip(singingClip));
                if (!singingClip->notes()->insertItem(newNote)) {
                    document->model()->destroyItem(newNote);
                    newNote = nullptr;
                    return false;
                }
                success = true;
                return true;
            });
            if (success && newNote) {
                document->selectionModel()->select(newNote, dspx::SelectionModel::Select | dspx::SelectionModel::SetCurrentItem | dspx::SelectionModel::ClearPreviousSelection);
            }
        });
    }

    PianoRollPanelInterface::PianoRollPanelInterface(Internal::PianoRollAddOn *addOn, Core::ProjectWindowInterface *windowHandle) : QObject(windowHandle), d_ptr(new PianoRollPanelInterfacePrivate) {
        Q_D(PianoRollPanelInterface);
        Q_ASSERT(windowHandle->getObjects(staticMetaObject.className()).isEmpty());
        windowHandle->addObject(staticMetaObject.className(), this);
        d->q_ptr = this;
        d->windowHandle = windowHandle;
        d->addon = addOn;

        d->timeViewModel = new sflow::TimeViewModel(this);
        d->timeLayoutViewModel = new sflow::TimeLayoutViewModel(this);
        d->scrollBehaviorViewModel = new sflow::ScrollBehaviorViewModel(this);
        d->timelineInteractionController = ProjectViewModelContext::of(d->windowHandle)->createAndBindTimelineInteractionController(this);
        d->labelSequenceInteractionControllerOfTempo = ProjectViewModelContext::of(d->windowHandle)->createAndBindLabelSequenceInteractionControllerOfTempo(this);
        d->labelSequenceInteractionControllerOfKeySignature = ProjectViewModelContext::of(d->windowHandle)->createAndBindLabelSequenceInteractionControllerOfKeySignature(this);
        d->labelSequenceInteractionControllerOfLabel = ProjectViewModelContext::of(d->windowHandle)->createAndBindLabelSequenceInteractionControllerOfLabel(this);
        d->noteEditLayerInteractionController = ProjectViewModelContext::of(d->windowHandle)->createAndBindNoteEditLayerInteractionController(this);
        d->clavierViewModel = new sflow::ClavierViewModel(this);
        d->clavierInteractionController = new sflow::ClavierInteractionController(this);

        d->positionAlignmentManipulator = new PositionAlignmentManipulator(this);
        d->positionAlignmentManipulator->setTimeLayoutViewModel(d->timeLayoutViewModel);
        d->autoPageScrollingManipulator = new AutoPageScrollingManipulator(this);
        d->autoPageScrollingManipulator->setEnabled(true);
        d->autoPageScrollingManipulator->setTimeViewModel(d->timeViewModel);
        d->autoPageScrollingManipulator->setTimeLayoutViewModel(d->timeLayoutViewModel);
        d->autoPageScrollingManipulator->setPlaybackViewModel(ProjectViewModelContext::of(d->windowHandle)->playbackViewModel());

        d->trackOverlaySelectorModel = new Internal::TrackOverlaySelectorModel(this);
        d->trackOverlaySelectorModel->setTrackList(windowHandle->projectDocumentContext()->document()->model()->tracks());

        d->singingClipListModel = new Internal::SingingClipListModel(this);

        d->editingClipSelectorModel = new QSortFilterProxyModel(this);
        d->editingClipSelectorModel->setSourceModel(d->singingClipListModel);
        d->editingClipSelectorModel->setDynamicSortFilter(true);
        d->editingClipSelectorModel->setSortRole(Internal::SingingClipListModel::ClipPositionRole);

        d->editingClipSelectionModel = new dspx::SelectionModel(d->windowHandle->projectDocumentContext()->document()->model(), this);

        {
            QQmlComponent component(Core::RuntimeInterface::qmlEngine(), "DiffScope.VisualEditor", "PianoRollView");
            if (component.isError()) {
                qFatal() << component.errorString();
            }
            auto o = component.createWithInitialProperties({
                {"addOn", QVariant::fromValue(d->addon)},
                {"pianoRollPanelInterface", QVariant::fromValue(this)}
            });
            if (component.isError()) {
                qFatal() << component.errorString();
            }
            o->setParent(this);
            d->pianoRollView = qobject_cast<QQuickItem *>(o);
            Q_ASSERT(d->pianoRollView);
        }

        d->autoPageScrollingManipulator->setTarget(d->pianoRollView->property("timeline").value<QQuickItem *>());

        connect(d->editingClipSelectionModel->clipSelectionModel(), &dspx::ClipSelectionModel::selectedItemsChanged, this, &PianoRollPanelInterface::editingClipChanged);

        connect(d->editingClipSelectionModel->clipSelectionModel(), &dspx::ClipSelectionModel::clipSequencesWithSelectedItemsChanged, this, [this] {
            Q_D(PianoRollPanelInterface);
            auto a = d->editingClipSelectionModel->clipSelectionModel()->clipSequencesWithSelectedItems();
            auto sequence = a.isEmpty() ? nullptr : a.first();
            d->singingClipListModel->setClipSequence(sequence);
        });

        auto noteSelectionModel = d->windowHandle->projectDocumentContext()->document()->selectionModel()->noteSelectionModel();
        connect(noteSelectionModel, &dspx::NoteSelectionModel::currentItemChanged, this, [=, this] {
            disconnect(d->currentNoteConnection);
            auto currentNote = noteSelectionModel->currentItem();
            if (currentNote) {
                setImplicitNoteLength(currentNote->length());
                d->currentNoteConnection = connect(currentNote, &dspx::Note::lengthChanged, this, [=, this] {
                    setImplicitNoteLength(currentNote->length());
                });
            }
        });

        Q_EMIT editingClipChanged();

        d->bindTimeViewModel();
        d->bindTimeLayoutViewModel();
        d->bindTimelineInteractionController();
        d->bindScrollBehaviorViewModel();
        d->bindPositionAlignmentManipulator();
        d->bindControllersInteraction();
        d->bindClavierInteractionController();
        d->bindNoteEditLayerInteractionController();

        connect(Internal::EditorPreference::instance(), &Internal::EditorPreference::trackCursorPositionChanged, this, [=, this] {
            setMouseTrackingDisabled(!Internal::EditorPreference::trackCursorPosition());
        });
    }

    PianoRollPanelInterface::~PianoRollPanelInterface() = default;

    PianoRollPanelInterface *PianoRollPanelInterface::of(const Core::ProjectWindowInterface *windowHandle) {
        return qobject_cast<PianoRollPanelInterface *>(windowHandle->getFirstObject(staticMetaObject.className()));
    }

    Core::ProjectWindowInterface *PianoRollPanelInterface::windowHandle() const {
        Q_D(const PianoRollPanelInterface);
        return d->windowHandle;
    }

    sflow::TimeViewModel *PianoRollPanelInterface::timeViewModel() const {
        Q_D(const PianoRollPanelInterface);
        return d->timeViewModel;
    }

    sflow::TimeLayoutViewModel *PianoRollPanelInterface::timeLayoutViewModel() const {
        Q_D(const PianoRollPanelInterface);
        return d->timeLayoutViewModel;
    }

    sflow::ScrollBehaviorViewModel *PianoRollPanelInterface::scrollBehaviorViewModel() const {
        Q_D(const PianoRollPanelInterface);
        return d->scrollBehaviorViewModel;
    }

    sflow::TimelineInteractionController *PianoRollPanelInterface::timelineInteractionController() const {
        Q_D(const PianoRollPanelInterface);
        return d->timelineInteractionController;
    }

    sflow::LabelSequenceInteractionController *PianoRollPanelInterface::labelSequenceInteractionControllerOfTempo() const {
        Q_D(const PianoRollPanelInterface);
        return d->labelSequenceInteractionControllerOfTempo;
    }

    sflow::LabelSequenceInteractionController *PianoRollPanelInterface::labelSequenceInteractionControllerOfKeySignature() const {
        Q_D(const PianoRollPanelInterface);
        return d->labelSequenceInteractionControllerOfKeySignature;
    }

    sflow::LabelSequenceInteractionController *PianoRollPanelInterface::labelSequenceInteractionControllerOfLabel() const {
        Q_D(const PianoRollPanelInterface);
        return d->labelSequenceInteractionControllerOfLabel;
    }

    sflow::NoteEditLayerInteractionController *PianoRollPanelInterface::noteEditLayerInteractionController() const {
        Q_D(const PianoRollPanelInterface);
        return d->noteEditLayerInteractionController;
    }

    sflow::ClavierViewModel *PianoRollPanelInterface::clavierViewModel() const {
        Q_D(const PianoRollPanelInterface);
        return d->clavierViewModel;
    }

    sflow::ClavierInteractionController *PianoRollPanelInterface::clavierInteractionController() const {
        Q_D(const PianoRollPanelInterface);
        return d->clavierInteractionController;
    }

    PositionAlignmentManipulator *PianoRollPanelInterface::positionAlignmentManipulator() const {
        Q_D(const PianoRollPanelInterface);
        return d->positionAlignmentManipulator;
    }

    AutoPageScrollingManipulator *PianoRollPanelInterface::autoPageScrollingManipulator() const {
        Q_D(const PianoRollPanelInterface);
        return d->autoPageScrollingManipulator;
    }

    QQuickItem *PianoRollPanelInterface::pianoRollView() const {
        Q_D(const PianoRollPanelInterface);
        return d->pianoRollView;
    }

    QAbstractItemModel *PianoRollPanelInterface::trackOverlaySelectorModel() const {
        Q_D(const PianoRollPanelInterface);
        return d->trackOverlaySelectorModel;
    }

    QAbstractItemModel *PianoRollPanelInterface::editingClipSelectorModel() const {
        Q_D(const PianoRollPanelInterface);
        return d->editingClipSelectorModel;
    }

    PianoRollPanelInterface::Tool PianoRollPanelInterface::tool() const {
        Q_D(const PianoRollPanelInterface);
        return d->tool;
    }

    void PianoRollPanelInterface::setTool(Tool tool) {
        Q_D(PianoRollPanelInterface);
        if (d->tool != tool) {
            d->tool = tool;
            Q_EMIT toolChanged();
        }
    }

    bool PianoRollPanelInterface::isSnapTemporarilyDisabled() const {
        Q_D(const PianoRollPanelInterface);
        return d->isSnapTemporarilyDisabled;
    }

    void PianoRollPanelInterface::setSnapTemporarilyDisabled(bool disabled) {
        Q_D(PianoRollPanelInterface);
        if (d->isSnapTemporarilyDisabled != disabled) {
            d->isSnapTemporarilyDisabled = disabled;
            Q_EMIT snapTemporarilyDisabledChanged();
        }
    }

    bool PianoRollPanelInterface::isMouseTrackingDisabled() const {
        Q_D(const PianoRollPanelInterface);
        return d->isMouseTrackingDisabled;
    }

    void PianoRollPanelInterface::setMouseTrackingDisabled(bool disabled) {
        Q_D(PianoRollPanelInterface);
        if (d->isMouseTrackingDisabled != disabled) {
            d->isMouseTrackingDisabled = disabled;
            Q_EMIT mouseTrackingDisabledChanged();
        }
    }

    dspx::SingingClip *PianoRollPanelInterface::editingClip() const {
        Q_D(const PianoRollPanelInterface);
        auto items = d->editingClipSelectionModel->clipSelectionModel()->selectedItems();
        if (items.isEmpty())
            return nullptr;
        Q_ASSERT(qobject_cast<dspx::SingingClip *>(items.first()));
        return static_cast<dspx::SingingClip *>(items.first());
    }

    void PianoRollPanelInterface::setEditingClip(dspx::SingingClip *clip) {
        Q_D(PianoRollPanelInterface);
        if (d->editingClipSelectionModel->currentItem() != clip) {
            qCInfo(lcPianoRollPanelInterface) << "Set editing clip to" << clip;
            d->editingClipSelectionModel->select(clip, dspx::SelectionModel::Select | dspx::SelectionModel::ClearPreviousSelection);
        }
    }

    int PianoRollPanelInterface::implicitNoteLength() const {
        Q_D(const PianoRollPanelInterface);
        return d->implicitNoteLength;
    }

    void PianoRollPanelInterface::setImplicitNoteLength(int length) {
        Q_D(PianoRollPanelInterface);
        if (d->implicitNoteLength != length) {
            d->implicitNoteLength = length;
            Q_EMIT implicitNoteLengthChanged();
        }
    }

}

#include "moc_PianoRollPanelInterface.cpp"
