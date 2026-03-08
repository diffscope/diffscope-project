#include "MixerPanelInterface.h"
#include "MixerPanelInterface_p.h"

#include <QQmlComponent>
#include <QQuickItem>
#include <QVariant>

#include <CoreApi/runtimeinterface.h>

#include <ScopicFlowCore/ScrollBehaviorViewModel.h>
#include <ScopicFlowCore/TrackListInteractionController.h>
#include <ScopicFlowCore/TrackListLayoutViewModel.h>

#include <coreplugin/ProjectWindowInterface.h>

#include <visualeditor/ProjectViewModelContext.h>
#include <visualeditor/internal/MixerAddOn.h>
#include <visualeditor/internal/EditorPreference.h>

namespace VisualEditor {

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

    static sflow::ScrollBehaviorViewModel::ScrollTypes getScrollTypes(MixerPanelInterface::Tool tool) {
        if (tool == MixerPanelInterface::HandTool) {
            return sflow::ScrollBehaviorViewModel::Wheel | sflow::ScrollBehaviorViewModel::Pinch | sflow::ScrollBehaviorViewModel::MiddleButton | sflow::ScrollBehaviorViewModel::LeftButton;
        }
        return sflow::ScrollBehaviorViewModel::Wheel | sflow::ScrollBehaviorViewModel::Pinch | sflow::ScrollBehaviorViewModel::MiddleButton;
    }

    void MixerPanelInterfacePrivate::bindScrollBehaviorViewModel() const {
        Q_Q(const MixerPanelInterface);
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
        QObject::connect(q, &MixerPanelInterface::toolChanged, scrollBehaviorViewModel, [=, this] {
            scrollBehaviorViewModel->setScrollTypes(getScrollTypes(tool));
        });
    }

    MixerPanelInterface::MixerPanelInterface(Internal::MixerAddOn *addOn, Core::ProjectWindowInterface *windowHandle) : QObject(windowHandle), d_ptr(new MixerPanelInterfacePrivate) {
        Q_D(MixerPanelInterface);
        Q_ASSERT(windowHandle->getObjects(staticMetaObject.className()).isEmpty());
        windowHandle->addObject(staticMetaObject.className(), this);
        d->q_ptr = this;
        d->windowHandle = windowHandle;
        d->addon = addOn;

        d->trackListLayoutViewModel = new sflow::TrackListLayoutViewModel(this);
        d->masterTrackListLayoutViewModel = new sflow::TrackListLayoutViewModel(this);
        d->scrollBehaviorViewModel = new sflow::ScrollBehaviorViewModel(this);
        d->trackListInteractionController = ProjectViewModelContext::of(d->windowHandle)->createAndBindTrackListInteractionController();
        d->masterTrackListInteractionController = ProjectViewModelContext::of(d->windowHandle)->createAndBindTrackListInteractionControllerOfMaster();

        QQmlComponent component(Core::RuntimeInterface::qmlEngine(), "DiffScope.VisualEditor", "MixerView");
        if (component.isError()) {
            qFatal() << component.errorString();
        }
        auto o = component.createWithInitialProperties({
            {"addOn", QVariant::fromValue(d->addon)},
            {"mixerPanelInterface", QVariant::fromValue(this)}
        });
        if (component.isError()) {
            qFatal() << component.errorString();
        }
        o->setParent(this);
        d->mixerView = qobject_cast<QQuickItem *>(o);
        Q_ASSERT(d->mixerView);

        d->bindScrollBehaviorViewModel();
    }

    MixerPanelInterface::~MixerPanelInterface() = default;

    MixerPanelInterface *MixerPanelInterface::of(const Core::ProjectWindowInterface *windowHandle) {
        return qobject_cast<MixerPanelInterface *>(windowHandle->getFirstObject(staticMetaObject.className()));
    }

    Core::ProjectWindowInterface *MixerPanelInterface::windowHandle() const {
        Q_D(const MixerPanelInterface);
        return d->windowHandle;
    }

    sflow::TrackListLayoutViewModel *MixerPanelInterface::trackListLayoutViewModel() const {
        Q_D(const MixerPanelInterface);
        return d->trackListLayoutViewModel;
    }

    sflow::TrackListLayoutViewModel *MixerPanelInterface::masterTrackListLayoutViewModel() const {
        Q_D(const MixerPanelInterface);
        return d->masterTrackListLayoutViewModel;
    }

    sflow::ScrollBehaviorViewModel *MixerPanelInterface::scrollBehaviorViewModel() const {
        Q_D(const MixerPanelInterface);
        return d->scrollBehaviorViewModel;
    }

    sflow::TrackListInteractionController *MixerPanelInterface::trackListInteractionController() const {
        Q_D(const MixerPanelInterface);
        return d->trackListInteractionController;
    }

    sflow::TrackListInteractionController *MixerPanelInterface::masterTrackListInteractionController() const {
        Q_D(const MixerPanelInterface);
        return d->masterTrackListInteractionController;
    }

    QQuickItem *MixerPanelInterface::mixerView() const {
        Q_D(const MixerPanelInterface);
        return d->mixerView;
    }

    MixerPanelInterface::Tool MixerPanelInterface::tool() const {
        Q_D(const MixerPanelInterface);
        return d->tool;
    }

    void MixerPanelInterface::setTool(Tool tool) {
        Q_D(MixerPanelInterface);
        if (d->tool != tool) {
            d->tool = tool;
            Q_EMIT toolChanged();
        }
    }

}

#include "moc_MixerPanelInterface.cpp"
