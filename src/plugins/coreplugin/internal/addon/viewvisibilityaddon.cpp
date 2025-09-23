#include "viewvisibilityaddon.h"

#include <QQmlComponent>

#include <SVSCraftQuick/MessageBox.h>

#include <QAKQuick/quickactioncontext.h>

#include <CoreApi/runtimeInterface.h>

#include <coreplugin/coreinterface.h>
#include <coreplugin/projectwindowinterface.h>

namespace Core::Internal {
    ViewVisibilityAddOn::ViewVisibilityAddOn(QObject *parent) : WindowInterfaceAddOn(parent) {
    }
    ViewVisibilityAddOn::~ViewVisibilityAddOn() = default;

    static const char settingCategoryC[] = "Core::Internal::ViewVisibilityAddOn";

    void ViewVisibilityAddOn::initialize() {
        auto settings = RuntimeInterface::settings();
        auto windowInterface = windowHandle()->cast<ProjectWindowInterface>();
        auto window = windowInterface->window();

        QQmlComponent component(RuntimeInterface::qmlEngine(), "DiffScope.Core", "ViewVisibilityAddOnActions");
        if (component.isError()) {
            qFatal() << component.errorString();
        }
        auto o = component.createWithInitialProperties({
            {"addOn", QVariant::fromValue(this)},
        });
        o->setParent(this);
        QMetaObject::invokeMethod(o, "registerToContext", windowInterface->actionContext());

        settings->beginGroup(settingCategoryC);

        auto menuBarVisible = !settings->value(QString::number(MenuBar)).value<bool>();
        auto menuBar = window->property("menuBar").value<QObject *>();
        menuBar->setProperty("visible", menuBarVisible);

        auto toolBarVisible = !settings->value(QString::number(ToolBar)).value<bool>();
        auto toolBar = window->property("toolBar").value<QObject *>();
        toolBar->setProperty("visible", toolBarVisible);

        auto leftSideBarVisible = !settings->value(QString::number(LeftSideBar)).value<bool>();
        auto leftDockingView = window->property("leftDockingView").value<QObject *>();
        leftDockingView->setProperty("barSize", leftSideBarVisible ? 32 : 0);

        auto rightSideBarVisible = !settings->value(QString::number(RightSideBar)).value<bool>();
        auto rightDockingView = window->property("rightDockingView").value<QObject *>();
        rightDockingView->setProperty("barSize", rightSideBarVisible ? 32 : 0);

        auto topSideBarVisible = !settings->value(QString::number(TopSideBar)).value<bool>();
        auto topDockingView = window->property("topDockingView").value<QObject *>();
        topDockingView->setProperty("barSize", topSideBarVisible ? 32 : 0);

        auto bottomSideBarVisible = !settings->value(QString::number(BottomSideBar)).value<bool>();
        auto bottomDockingView = window->property("bottomDockingView").value<QObject *>();
        bottomDockingView->setProperty("barSize", bottomSideBarVisible ? 32 : 0);

        auto statusBarVisible = !settings->value(QString::number(StatusBar)).value<bool>();
        auto statusBar = window->property("statusBar").value<QObject *>();
        statusBar->setProperty("visible", statusBarVisible);

        settings->endGroup();
    }
    void ViewVisibilityAddOn::extensionsInitialized() {
    }
    bool ViewVisibilityAddOn::delayedInitialize() {
        return WindowInterfaceAddOn::delayedInitialize();
    }
    void ViewVisibilityAddOn::toggleVisibility(ViewVisibilityOption option, bool visible, QObject *action) const {
        auto settings = RuntimeInterface::settings();
        auto window = windowHandle()->window();
        settings->beginGroup(settingCategoryC);
        if (option == MenuBar) {
            auto menuBar = window->property("menuBar").value<QObject *>();
            if (!visible) {
                if (SVS::SVSCraft::No ==
                    SVS::MessageBox::warning(
                        RuntimeInterface::qmlEngine(), window, tr("Please take attention"),
                        tr("After hiding the menu bar, it can be difficult to show it again. Make "
                           "sure you know how to do this.\n\nContinue?"),
                        SVS::SVSCraft::Yes | SVS::SVSCraft::No, SVS::SVSCraft::No)) {
                    if (action)
                        action->setProperty("checked", true);
                    goto end;
                }
            }
            menuBar->setProperty("visible", visible);
        } else if (option == ToolBar) {
            auto toolBar = window->property("toolBar").value<QObject *>();
            toolBar->setProperty("visible", visible);
        } else if (option == LeftSideBar) {
            auto leftDockingView = window->property("leftDockingView").value<QObject *>();
            leftDockingView->setProperty("barSize", visible ? 32 : 0);
        } else if (option == RightSideBar) {
            auto rightDockingView = window->property("rightDockingView").value<QObject *>();
            rightDockingView->setProperty("barSize", visible ? 32 : 0);
        } else if (option == TopSideBar) {
            auto topDockingView = window->property("topDockingView").value<QObject *>();
            topDockingView->setProperty("barSize", visible ? 32 : 0);
        } else if (option == BottomSideBar) {
            auto bottomDockingView = window->property("bottomDockingView").value<QObject *>();
            bottomDockingView->setProperty("barSize", visible ? 32 : 0);
        } else if (option == StatusBar) {
            auto statusBar = window->property("statusBar").value<QObject *>();
            statusBar->setProperty("visible", visible);
        }
        settings->setValue(QString::number(option), !visible);
    end:
        settings->endGroup();
    }
}
