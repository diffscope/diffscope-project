#include "ArrangementPanelInterface.h"
#include "ArrangementPanelInterface_p.h"

#include <cmath>

#include <QQmlComponent>
#include <QQuickItem>

#include <CoreApi/runtimeinterface.h>

#include <ScopicFlowCore/ScrollBehaviorViewModel.h>
#include <ScopicFlowCore/TimeLayoutViewModel.h>
#include <ScopicFlowCore/TimeViewModel.h>
#include <ScopicFlowCore/TimelineInteractionController.h>

#include <coreplugin/ProjectWindowInterface.h>
#include <coreplugin/ProjectTimeline.h>

#include <visualeditor/PositionAlignmentManipulator.h>
#include <visualeditor/ProjectViewModelContext.h>
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

    ArrangementPanelInterface::ArrangementPanelInterface(Core::ProjectWindowInterface *windowHandle) : QObject(windowHandle), d_ptr(new ArrangementPanelInterfacePrivate) {
        Q_D(ArrangementPanelInterface);
        Q_ASSERT(windowHandle->getObjects(staticMetaObject.className()).isEmpty());
        windowHandle->addObject(staticMetaObject.className(), this);
        d->q_ptr = this;
        d->windowHandle = windowHandle;

        d->timeViewModel = new sflow::TimeViewModel(this);
        d->timeLayoutViewModel = new sflow::TimeLayoutViewModel(this);
        d->timelineInteractionController = new sflow::TimelineInteractionController(this);
        d->scrollBehaviorViewModel = new sflow::ScrollBehaviorViewModel(this);

        d->positionAlignmentManipulator = new PositionAlignmentManipulator(this);
        d->positionAlignmentManipulator->setTimeLayoutViewModel(d->timeLayoutViewModel);

        QQmlComponent component(Core::RuntimeInterface::qmlEngine(), "DiffScope.VisualEditor", "ArrangementView");
        if (component.isError()) {
            qFatal() << component.errorString();
        }
        auto o = component.createWithInitialProperties({
            {"arrangementPanelInterface", QVariant::fromValue(this)},
            {"projectViewModelContext", QVariant::fromValue(ProjectViewModelContext::of(d->windowHandle))},
        });
        if (component.isError()) {
            qFatal() << component.errorString();
        }
        o->setParent(this);
        d->arrangementView = qobject_cast<QQuickItem *>(o);
        Q_ASSERT(d->arrangementView);

        d->bindTimeViewModel();
        d->bindTimeLayoutViewModel();
        d->bindTimelineInteractionController();
        d->bindScrollBehaviorViewModel();
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
    sflow::ScrollBehaviorViewModel *ArrangementPanelInterface::scrollBehaviorViewModel() const {
        Q_D(const ArrangementPanelInterface);
        return d->scrollBehaviorViewModel;
    }
    sflow::TimelineInteractionController *ArrangementPanelInterface::timelineInteractionController() const {
        Q_D(const ArrangementPanelInterface);
        return d->timelineInteractionController;
    }

    PositionAlignmentManipulator *ArrangementPanelInterface::positionAlignmentManipulator() const {
        Q_D(const ArrangementPanelInterface);
        return d->positionAlignmentManipulator;
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

}

#include "moc_ArrangementPanelInterface.cpp"