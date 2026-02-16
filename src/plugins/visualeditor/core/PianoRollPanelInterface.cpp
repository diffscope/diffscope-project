#include "PianoRollPanelInterface.h"
#include "PianoRollPanelInterface_p.h"

#include <cmath>

#include <QQmlComponent>
#include <QQuickItem>
#include <QVariant>

#include <CoreApi/runtimeinterface.h>

#include <ScopicFlowCore/ClavierViewModel.h>
#include <ScopicFlowCore/ClavierInteractionController.h>
#include <ScopicFlowCore/LabelSequenceInteractionController.h>
#include <ScopicFlowCore/ScrollBehaviorViewModel.h>
#include <ScopicFlowCore/TimeLayoutViewModel.h>
#include <ScopicFlowCore/TimeViewModel.h>
#include <ScopicFlowCore/TimelineInteractionController.h>

#include <dspxmodel/Model.h>
#include <dspxmodel/TrackList.h>

#include <coreplugin/ProjectTimeline.h>
#include <coreplugin/ProjectWindowInterface.h>
#include <coreplugin/ProjectDocumentContext.h>
#include <coreplugin/DspxDocument.h>

#include <visualeditor/AutoPageScrollingManipulator.h>
#include <visualeditor/PositionAlignmentManipulator.h>
#include <visualeditor/ProjectViewModelContext.h>
#include <visualeditor/internal/EditorPreference.h>
#include <visualeditor/internal/PianoRollAddOn.h>

namespace VisualEditor {

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
                positionAlignmentManipulator->setDuration(PositionAlignmentManipulator::Unset);
            } else {
                positionAlignmentManipulator->setDuration(previousDuration);
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
                    break;
                }
                case PianoRollPanelInterface::HandTool:
                    break;
            }
        });
    }

    void PianoRollPanelInterfacePrivate::bindClavierInteractionController() const {
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
        d->labelSequenceInteractionControllerOfLabel = ProjectViewModelContext::of(d->windowHandle)->createAndBindLabelSequenceInteractionControllerOfLabel(this);
        d->clavierViewModel = new sflow::ClavierViewModel(this);
        d->clavierInteractionController = new sflow::ClavierInteractionController(this);

        d->positionAlignmentManipulator = new PositionAlignmentManipulator(this);
        d->positionAlignmentManipulator->setTimeLayoutViewModel(d->timeLayoutViewModel);
        d->autoPageScrollingManipulator = new AutoPageScrollingManipulator(this);
        d->autoPageScrollingManipulator->setEnabled(true);
        d->autoPageScrollingManipulator->setTimeViewModel(d->timeViewModel);
        d->autoPageScrollingManipulator->setTimeLayoutViewModel(d->timeLayoutViewModel);
        d->autoPageScrollingManipulator->setPlaybackViewModel(ProjectViewModelContext::of(d->windowHandle)->playbackViewModel());

        {
            auto tracks = windowHandle->projectDocumentContext()->document()->model()->tracks();
            QQmlComponent component(Core::RuntimeInterface::qmlEngine(), "DiffScope.VisualEditor", "TrackOverlaySelectorModel");
            if (component.isError()) {
                qFatal() << component.errorString();
            }
            auto o = component.createWithInitialProperties({
                {"trackList", QVariant::fromValue(tracks)},
            });
            if (component.isError()) {
                qFatal() << component.errorString();
            }
            o->setParent(this);
            d->trackOverlaySelectorModel = o;
        }

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

        d->bindTimeViewModel();
        d->bindTimeLayoutViewModel();
        d->bindTimelineInteractionController();
        d->bindScrollBehaviorViewModel();
        d->bindPositionAlignmentManipulator();
        d->bindControllersInteraction();
        d->bindClavierInteractionController();

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

    sflow::LabelSequenceInteractionController *PianoRollPanelInterface::labelSequenceInteractionControllerOfLabel() const {
        Q_D(const PianoRollPanelInterface);
        return d->labelSequenceInteractionControllerOfLabel;
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

    QObject *PianoRollPanelInterface::trackOverlaySelectorModel() const {
        Q_D(const PianoRollPanelInterface);
        return d->trackOverlaySelectorModel;
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

}

#include "moc_PianoRollPanelInterface.cpp"
