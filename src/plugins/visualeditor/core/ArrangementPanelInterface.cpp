#include "ArrangementPanelInterface.h"
#include "ArrangementPanelInterface_p.h"

#include <cmath>

#include <QQmlComponent>
#include <QQuickItem>

#include <CoreApi/runtimeinterface.h>

#include <ScopicFlowCore/ClipPaneInteractionController.h>
#include <ScopicFlowCore/LabelSequenceInteractionController.h>
#include <ScopicFlowCore/ScrollBehaviorViewModel.h>
#include <ScopicFlowCore/TimeLayoutViewModel.h>
#include <ScopicFlowCore/TimeViewModel.h>
#include <ScopicFlowCore/TimelineInteractionController.h>
#include <ScopicFlowCore/TrackListInteractionController.h>
#include <ScopicFlowCore/TrackListLayoutViewModel.h>

#include <coreplugin/ProjectTimeline.h>
#include <coreplugin/ProjectWindowInterface.h>

#include <visualeditor/AutoPageScrollingManipulator.h>
#include <visualeditor/PositionAlignmentManipulator.h>
#include <visualeditor/ProjectViewModelContext.h>
#include <visualeditor/internal/ArrangementAddOn.h>
#include <visualeditor/internal/EditorPreference.h>

namespace VisualEditor {

    void ArrangementPanelInterfacePrivate::bindTimeViewModel() const {
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
    void ArrangementPanelInterfacePrivate::bindTimeLayoutViewModel() const {
    }
    void ArrangementPanelInterfacePrivate::bindTimelineInteractionController() const {

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

    static sflow::ScrollBehaviorViewModel::ScrollTypes getScrollTypes(ArrangementPanelInterface::Tool tool) {
        if (tool == ArrangementPanelInterface::HandTool) {
            return sflow::ScrollBehaviorViewModel::Wheel | sflow::ScrollBehaviorViewModel::Pinch | sflow::ScrollBehaviorViewModel::MiddleButton | sflow::ScrollBehaviorViewModel::LeftButton;
        } else {
            return sflow::ScrollBehaviorViewModel::Wheel | sflow::ScrollBehaviorViewModel::Pinch | sflow::ScrollBehaviorViewModel::MiddleButton;
        }
    }

    void ArrangementPanelInterfacePrivate::bindScrollBehaviorViewModel() const {
        Q_Q(const ArrangementPanelInterface);
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
        QObject::connect(q, &ArrangementPanelInterface::toolChanged, scrollBehaviorViewModel, [=, this] {
            scrollBehaviorViewModel->setScrollTypes(getScrollTypes(tool));
        });
    }

    void ArrangementPanelInterfacePrivate::bindPositionAlignmentManipulator() const {
        Q_Q(const ArrangementPanelInterface);
        positionAlignmentManipulator->setAutoDurationPositionAlignment(Internal::EditorPreference::autoDurationPositionAlignment());
        QObject::connect(Internal::EditorPreference::instance(), &Internal::EditorPreference::autoDurationPositionAlignmentChanged, positionAlignmentManipulator, [=, this] {
            positionAlignmentManipulator->setAutoDurationPositionAlignment(Internal::EditorPreference::autoDurationPositionAlignment());
        });
        QObject::connect(q, &ArrangementPanelInterface::snapTemporarilyDisabledChanged, positionAlignmentManipulator, [=, this] {
            if (isSnapTemporarilyDisabled) {
                previousDuration = positionAlignmentManipulator->duration();
                positionAlignmentManipulator->setDuration(PositionAlignmentManipulator::Unset);
            } else {
                positionAlignmentManipulator->setDuration(previousDuration);
            }
        });
    }

    void ArrangementPanelInterfacePrivate::bindControllersInteraction() const {
        Q_Q(const ArrangementPanelInterface);
        QObject::connect(q, &ArrangementPanelInterface::toolChanged, q, [=, this] {
            switch (tool) {
                case ArrangementPanelInterface::PointerTool: {
                    labelSequenceInteractionControllerOfLabel->setPrimaryItemInteraction(sflow::LabelSequenceInteractionController::Move);
                    labelSequenceInteractionControllerOfLabel->setSecondaryItemInteraction(sflow::LabelSequenceInteractionController::CopyAndMove);
                    labelSequenceInteractionControllerOfLabel->setPrimarySceneInteraction(sflow::LabelSequenceInteractionController::RubberBandSelect);
                    labelSequenceInteractionControllerOfLabel->setSecondarySceneInteraction(sflow::LabelSequenceInteractionController::RubberBandSelect);

                    labelSequenceInteractionControllerOfTempo->setPrimaryItemInteraction(sflow::LabelSequenceInteractionController::Move);
                    labelSequenceInteractionControllerOfTempo->setSecondaryItemInteraction(sflow::LabelSequenceInteractionController::Move);
                    labelSequenceInteractionControllerOfTempo->setPrimarySceneInteraction(sflow::LabelSequenceInteractionController::RubberBandSelect);
                    labelSequenceInteractionControllerOfTempo->setSecondarySceneInteraction(sflow::LabelSequenceInteractionController::RubberBandSelect);

                    trackListInteractionController->setPrimaryItemInteraction(sflow::TrackListInteractionController::DragMove);
                    trackListInteractionController->setSecondaryItemInteraction(sflow::TrackListInteractionController::DragCopy);
                    trackListInteractionController->setPrimarySceneInteraction(sflow::TrackListInteractionController::RubberBandSelect);
                    trackListInteractionController->setSecondarySceneInteraction(sflow::TrackListInteractionController::RubberBandSelect);

                    clipPaneInteractionController->setPrimaryItemInteraction(sflow::ClipPaneInteractionController::Move);
                    clipPaneInteractionController->setSecondaryItemInteraction(sflow::ClipPaneInteractionController::CopyAndMove);
                    clipPaneInteractionController->setPrimarySceneInteraction(sflow::ClipPaneInteractionController::RubberBandSelect);
                    clipPaneInteractionController->setSecondarySceneInteraction(sflow::ClipPaneInteractionController::TimeRangeSelect);
                    break;
                }
                case ArrangementPanelInterface::PencilTool: {
                    labelSequenceInteractionControllerOfLabel->setPrimaryItemInteraction(sflow::LabelSequenceInteractionController::Move);
                    labelSequenceInteractionControllerOfLabel->setSecondaryItemInteraction(sflow::LabelSequenceInteractionController::CopyAndMove);
                    labelSequenceInteractionControllerOfLabel->setPrimarySceneInteraction(sflow::LabelSequenceInteractionController::RubberBandSelect);
                    labelSequenceInteractionControllerOfLabel->setSecondarySceneInteraction(sflow::LabelSequenceInteractionController::RubberBandSelect);

                    labelSequenceInteractionControllerOfTempo->setPrimaryItemInteraction(sflow::LabelSequenceInteractionController::Move);
                    labelSequenceInteractionControllerOfTempo->setSecondaryItemInteraction(sflow::LabelSequenceInteractionController::Move);
                    labelSequenceInteractionControllerOfTempo->setPrimarySceneInteraction(sflow::LabelSequenceInteractionController::RubberBandSelect);
                    labelSequenceInteractionControllerOfTempo->setSecondarySceneInteraction(sflow::LabelSequenceInteractionController::RubberBandSelect);

                    trackListInteractionController->setPrimaryItemInteraction(sflow::TrackListInteractionController::DragMove);
                    trackListInteractionController->setSecondaryItemInteraction(sflow::TrackListInteractionController::DragCopy);
                    trackListInteractionController->setPrimarySceneInteraction(sflow::TrackListInteractionController::RubberBandSelect);
                    trackListInteractionController->setSecondarySceneInteraction(sflow::TrackListInteractionController::RubberBandSelect);

                    clipPaneInteractionController->setPrimaryItemInteraction(sflow::ClipPaneInteractionController::Move);
                    clipPaneInteractionController->setSecondaryItemInteraction(sflow::ClipPaneInteractionController::Draw);
                    clipPaneInteractionController->setPrimarySceneInteraction(sflow::ClipPaneInteractionController::Draw);
                    clipPaneInteractionController->setSecondarySceneInteraction(sflow::ClipPaneInteractionController::Draw);
                    break;
                }
                case ArrangementPanelInterface::SelectTool: {
                    labelSequenceInteractionControllerOfLabel->setPrimaryItemInteraction(sflow::LabelSequenceInteractionController::RubberBandSelect);
                    labelSequenceInteractionControllerOfLabel->setSecondaryItemInteraction(sflow::LabelSequenceInteractionController::RubberBandSelect);
                    labelSequenceInteractionControllerOfLabel->setPrimarySceneInteraction(sflow::LabelSequenceInteractionController::RubberBandSelect);
                    labelSequenceInteractionControllerOfLabel->setSecondarySceneInteraction(sflow::LabelSequenceInteractionController::RubberBandSelect);

                    labelSequenceInteractionControllerOfTempo->setPrimaryItemInteraction(sflow::LabelSequenceInteractionController::RubberBandSelect);
                    labelSequenceInteractionControllerOfTempo->setSecondaryItemInteraction(sflow::LabelSequenceInteractionController::RubberBandSelect);
                    labelSequenceInteractionControllerOfTempo->setPrimarySceneInteraction(sflow::LabelSequenceInteractionController::RubberBandSelect);
                    labelSequenceInteractionControllerOfTempo->setSecondarySceneInteraction(sflow::LabelSequenceInteractionController::RubberBandSelect);

                    trackListInteractionController->setPrimaryItemInteraction(sflow::TrackListInteractionController::RubberBandSelect);
                    trackListInteractionController->setSecondaryItemInteraction(sflow::TrackListInteractionController::RubberBandSelect);
                    trackListInteractionController->setPrimarySceneInteraction(sflow::TrackListInteractionController::RubberBandSelect);
                    trackListInteractionController->setSecondarySceneInteraction(sflow::TrackListInteractionController::RubberBandSelect);

                    clipPaneInteractionController->setPrimaryItemInteraction(sflow::ClipPaneInteractionController::RubberBandSelect);
                    clipPaneInteractionController->setSecondaryItemInteraction(sflow::ClipPaneInteractionController::TimeRangeSelect);
                    clipPaneInteractionController->setPrimarySceneInteraction(sflow::ClipPaneInteractionController::RubberBandSelect);
                    clipPaneInteractionController->setSecondarySceneInteraction(sflow::ClipPaneInteractionController::TimeRangeSelect);
                    break;
                }
                case ArrangementPanelInterface::HandTool:
                    break;
            }
        });
    }

    ArrangementPanelInterface::ArrangementPanelInterface(Internal::ArrangementAddOn *addOn, Core::ProjectWindowInterface *windowHandle) : QObject(windowHandle), d_ptr(new ArrangementPanelInterfacePrivate) {
        Q_D(ArrangementPanelInterface);
        Q_ASSERT(windowHandle->getObjects(staticMetaObject.className()).isEmpty());
        windowHandle->addObject(staticMetaObject.className(), this);
        d->q_ptr = this;
        d->windowHandle = windowHandle;
        d->addon = addOn;

        d->timeViewModel = new sflow::TimeViewModel(this);
        d->timeLayoutViewModel = new sflow::TimeLayoutViewModel(this);
        d->trackListLayoutViewModel = new sflow::TrackListLayoutViewModel(this);
        d->scrollBehaviorViewModel = new sflow::ScrollBehaviorViewModel(this);
        d->timelineInteractionController = ProjectViewModelContext::of(d->windowHandle)->createAndBindTimelineInteractionController(this);
        d->labelSequenceInteractionControllerOfTempo = ProjectViewModelContext::of(d->windowHandle)->createAndBindLabelSequenceInteractionControllerOfTempo(this);
        d->labelSequenceInteractionControllerOfLabel = ProjectViewModelContext::of(d->windowHandle)->createAndBindLabelSequenceInteractionControllerOfLabel(this);
        d->trackListInteractionController = ProjectViewModelContext::of(d->windowHandle)->createAndBindTrackListInteractionController(this);
        d->clipPaneInteractionController = ProjectViewModelContext::of(d->windowHandle)->createAndBindClipPaneInteractionController(this);

        d->positionAlignmentManipulator = new PositionAlignmentManipulator(this);
        d->positionAlignmentManipulator->setTimeLayoutViewModel(d->timeLayoutViewModel);
        d->autoPageScrollingManipulator = new AutoPageScrollingManipulator(this);
        d->autoPageScrollingManipulator->setEnabled(true);
        d->autoPageScrollingManipulator->setTimeViewModel(d->timeViewModel);
        d->autoPageScrollingManipulator->setTimeLayoutViewModel(d->timeLayoutViewModel);
        d->autoPageScrollingManipulator->setPlaybackViewModel(ProjectViewModelContext::of(d->windowHandle)->playbackViewModel());

        QQmlComponent component(Core::RuntimeInterface::qmlEngine(), "DiffScope.VisualEditor", "ArrangementView");
        if (component.isError()) {
            qFatal() << component.errorString();
        }
        auto o = component.createWithInitialProperties({
            {"addOn", QVariant::fromValue(d->addon)},
            {"arrangementPanelInterface", QVariant::fromValue(this)}
        });
        if (component.isError()) {
            qFatal() << component.errorString();
        }
        o->setParent(this);
        d->arrangementView = qobject_cast<QQuickItem *>(o);
        Q_ASSERT(d->arrangementView);

        d->autoPageScrollingManipulator->setTarget(d->arrangementView->property("timeline").value<QQuickItem *>());

        d->bindTimeViewModel();
        d->bindTimeLayoutViewModel();
        d->bindTimelineInteractionController();
        d->bindScrollBehaviorViewModel();
        d->bindPositionAlignmentManipulator();
        d->bindControllersInteraction();

        connect(Internal::EditorPreference::instance(), &Internal::EditorPreference::trackCursorPositionChanged, this, [=, this] {
            setMouseTrackingDisabled(!Internal::EditorPreference::trackCursorPosition());
        });
    }

    ArrangementPanelInterface::~ArrangementPanelInterface() = default;

    ArrangementPanelInterface *ArrangementPanelInterface::of(const Core::ProjectWindowInterface *windowHandle) {
        return qobject_cast<ArrangementPanelInterface *>(windowHandle->getFirstObject(staticMetaObject.className()));
    }

    Core::ProjectWindowInterface *ArrangementPanelInterface::windowHandle() const {
        Q_D(const ArrangementPanelInterface);
        return d->windowHandle;
    }

    sflow::TimeViewModel *ArrangementPanelInterface::timeViewModel() const {
        Q_D(const ArrangementPanelInterface);
        return d->timeViewModel;
    }
    sflow::TimeLayoutViewModel *ArrangementPanelInterface::timeLayoutViewModel() const {
        Q_D(const ArrangementPanelInterface);
        return d->timeLayoutViewModel;
    }

    sflow::TrackListLayoutViewModel *ArrangementPanelInterface::trackListLayoutViewModel() const {
        Q_D(const ArrangementPanelInterface);
        return d->trackListLayoutViewModel;
    }

    sflow::ScrollBehaviorViewModel *ArrangementPanelInterface::scrollBehaviorViewModel() const {
        Q_D(const ArrangementPanelInterface);
        return d->scrollBehaviorViewModel;
    }
    sflow::TimelineInteractionController *ArrangementPanelInterface::timelineInteractionController() const {
        Q_D(const ArrangementPanelInterface);
        return d->timelineInteractionController;
    }
    sflow::LabelSequenceInteractionController *ArrangementPanelInterface::labelSequenceInteractionControllerOfTempo() const {
        Q_D(const ArrangementPanelInterface);
        return d->labelSequenceInteractionControllerOfTempo;
    }

    sflow::LabelSequenceInteractionController *ArrangementPanelInterface::labelSequenceInteractionControllerOfLabel() const {
        Q_D(const ArrangementPanelInterface);
        return d->labelSequenceInteractionControllerOfLabel;
    }

    sflow::TrackListInteractionController *ArrangementPanelInterface::trackListInteractionController() const {
        Q_D(const ArrangementPanelInterface);
        return d->trackListInteractionController;
    }

    sflow::ClipPaneInteractionController *ArrangementPanelInterface::clipPaneInteractionController() const {
        Q_D(const ArrangementPanelInterface);
        return d->clipPaneInteractionController;
    }

    PositionAlignmentManipulator *ArrangementPanelInterface::positionAlignmentManipulator() const {
        Q_D(const ArrangementPanelInterface);
        return d->positionAlignmentManipulator;
    }

    AutoPageScrollingManipulator *ArrangementPanelInterface::autoPageScrollingManipulator() const {
        Q_D(const ArrangementPanelInterface);
        return d->autoPageScrollingManipulator;
    }

    QQuickItem *ArrangementPanelInterface::arrangementView() const {
        Q_D(const ArrangementPanelInterface);
        return d->arrangementView;
    }

    ArrangementPanelInterface::Tool ArrangementPanelInterface::tool() const {
        Q_D(const ArrangementPanelInterface);
        return d->tool;
    }

    void ArrangementPanelInterface::setTool(Tool tool) {
        Q_D(ArrangementPanelInterface);
        if (d->tool != tool) {
            d->tool = tool;
            Q_EMIT toolChanged();
        }
    }

    bool ArrangementPanelInterface::isSnapTemporarilyDisabled() const {
        Q_D(const ArrangementPanelInterface);
        return d->isSnapTemporarilyDisabled;
    }

    void ArrangementPanelInterface::setSnapTemporarilyDisabled(bool disabled) {
        Q_D(ArrangementPanelInterface);
        if (d->isSnapTemporarilyDisabled != disabled) {
            d->isSnapTemporarilyDisabled = disabled;
            Q_EMIT snapTemporarilyDisabledChanged();
        }
    }
    bool ArrangementPanelInterface::isMouseTrackingDisabled() const {
        Q_D(const ArrangementPanelInterface);
        return d->isMouseTrackingDisabled;
    }
    void ArrangementPanelInterface::setMouseTrackingDisabled(bool disabled) {
        Q_D(ArrangementPanelInterface);
        if (d->isMouseTrackingDisabled != disabled) {
            d->isMouseTrackingDisabled = disabled;
            Q_EMIT mouseTrackingDisabledChanged();
        }
    }

}

#include "moc_ArrangementPanelInterface.cpp"
