#include "ProjectWindowNavigatorAddOn.h"

#include <algorithm>
#include <ranges>

#include <QLoggingCategory>
#include <QQmlComponent>

#include <CoreApi/runtimeinterface.h>
#include <CoreApi/windowsystem.h>

#include <SVSCraftCore/SVSCraftNamespace.h>

#include <coreplugin/CoreInterface.h>
#include <coreplugin/ProjectWindowInterface.h>

namespace Core::Internal {

    Q_STATIC_LOGGING_CATEGORY(lcProjectWindowNavigatorAddOn, "diffscope.core.projectwindownavigatoraddon")

    ProjectWindowNavigatorAddOn::ProjectWindowNavigatorAddOn(QObject *parent) : WindowInterfaceAddOn(parent) {
    }

    ProjectWindowNavigatorAddOn::~ProjectWindowNavigatorAddOn() = default;

    void ProjectWindowNavigatorAddOn::initialize() {
        auto windowInterface = windowHandle()->cast<ActionWindowInterfaceBase>();
        QQmlComponent component(RuntimeInterface::qmlEngine(), "DiffScope.Core", "ProjectWindowNavigatorAddOnActions");
        if (component.isError()) {
            qFatal() << component.errorString();
        }
        auto o = component.createWithInitialProperties({
            {"addOn", QVariant::fromValue(this)},
        });
        o->setParent(this);
        QMetaObject::invokeMethod(o, "registerToContext", windowInterface->actionContext());
    }

    void ProjectWindowNavigatorAddOn::extensionsInitialized() {
        auto windowSystem = CoreInterface::windowSystem();

        // Connect to window creation and destruction signals
        connect(windowSystem, &WindowSystem::windowCreated, this, [this](WindowInterface *windowInterface) {
            if (qobject_cast<ProjectWindowInterface *>(windowInterface)) {
                updateProjectWindows();
            }
        });

        connect(windowSystem, &WindowSystem::windowAboutToDestroy, this, [this](WindowInterface *windowInterface) {
            if (qobject_cast<ProjectWindowInterface *>(windowInterface)) {
                updateProjectWindows();
            }
        });

        // Initialize the list with current windows
        updateProjectWindows();
    }

    bool ProjectWindowNavigatorAddOn::delayedInitialize() {
        return WindowInterfaceAddOn::delayedInitialize();
    }

    QList<ProjectWindowInterface *> ProjectWindowNavigatorAddOn::projectWindows() const {
        return m_projectWindows;
    }

    void ProjectWindowNavigatorAddOn::raiseWindow(ProjectWindowInterface *windowInterface) {
        qCDebug(lcProjectWindowNavigatorAddOn) << "Raising project window" << windowInterface;
        auto window = windowInterface->window();
        if (window->visibility() == QWindow::Minimized) {
            window->showNormal();
        }
        window->raise(); // TODO: what does the previous QMView::raiseWindow do to the window?
        window->requestActivate();
    }

    void ProjectWindowNavigatorAddOn::navigateToWindow(int step) const {
        if (auto windowInterface = qobject_cast<ProjectWindowInterface *>(windowHandle())) {
            auto index = m_projectWindows.indexOf(windowInterface);
            if (index == -1)
                return;
            index = (index + step + m_projectWindows.size()) % m_projectWindows.size();
            auto target = m_projectWindows[index];
            raiseWindow(target);
        } else if (!m_projectWindows.isEmpty()) {
            auto target = m_projectWindows.first();
            raiseWindow(target);
        }
    }

    void ProjectWindowNavigatorAddOn::updateProjectWindows() {
        m_projectWindows.clear();
        auto windows = CoreInterface::windowSystem()->windows();
        std::ranges::transform(
            windows | std::views::filter([](auto w) { return qobject_cast<ProjectWindowInterface *>(w); }),
            std::back_inserter(m_projectWindows),
            [](auto w) { return qobject_cast<ProjectWindowInterface *>(w); }
        );
        Q_EMIT projectWindowsChanged();
    }

}

#include "moc_ProjectWindowNavigatorAddOn.cpp"
