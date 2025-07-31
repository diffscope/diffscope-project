#include "projectaddon.h"

#include <QQmlComponent>

#include <QAKQuick/quickactioncontext.h>

#include <coreplugin/icore.h>
#include <coreplugin/iprojectwindow.h>

namespace Core::Internal {
    ProjectAddon::ProjectAddon(QObject *parent) : IWindowAddOn(parent) {
    }
    ProjectAddon::~ProjectAddon() = default;
    void ProjectAddon::initialize() {
        QQmlComponent component(ICore::qmlEngine(), "DiffScope.CorePlugin", "Actions");
        if (component.isError()) {
            qFatal() << component.errorString();
        }
        auto o = component.create();
        o->setParent(this);
        auto iWin = windowHandle()->cast<IProjectWindow>();
        iWin->actionContext()->addAction("core.newFile", o->property("newFile").value<QQmlComponent *>());
        iWin->actionContext()->addAction("core.openFile", o->property("openFile").value<QQmlComponent *>());
        iWin->actionContext()->addAction("core.settings", o->property("settings").value<QQmlComponent *>());
        iWin->actionContext()->addAction("core.plugins", o->property("plugins").value<QQmlComponent *>());
        iWin->actionContext()->addAction("core.workspaceLayouts", o->property("workspaceLayouts").value<QQmlComponent *>());
        iWin->actionContext()->addAction("core.workspacePanels", o->property("workspacePanels").value<QQmlComponent *>());
        iWin->actionContext()->addAction("core.dockActionToSideBar", o->property("dockActionToSideBar").value<QQmlComponent *>());
        iWin->actionContext()->addAction("core.documentations", o->property("documentations").value<QQmlComponent *>());
        iWin->actionContext()->addAction("core.findActions", o->property("findActions").value<QQmlComponent *>());
        iWin->actionContext()->addAction("core.aboutApp", o->property("aboutApp").value<QQmlComponent *>());
        iWin->actionContext()->addAction("core.aboutQt", o->property("aboutQt").value<QQmlComponent *>());

        iWin->actionContext()->addAction("core.arrangementPanel", new QQmlComponent(ICore::qmlEngine(), "DiffScope.CorePlugin", "ArrangementPanel", this));
        iWin->actionContext()->addAction("core.pianoRollPanel", new QQmlComponent(ICore::qmlEngine(), "DiffScope.CorePlugin", "PianoRollPanel", this));
        iWin->actionContext()->addAction("core.notificationsPanel", new QQmlComponent(ICore::qmlEngine(), "DiffScope.CorePlugin", "NotificationsPanel", this));
    }
    void ProjectAddon::extensionsInitialized() {
    }
    bool ProjectAddon::delayedInitialize() {
        return IWindowAddOn::delayedInitialize();
    }
}
