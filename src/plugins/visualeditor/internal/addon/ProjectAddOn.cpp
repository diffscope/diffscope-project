#include "ProjectAddOn.h"

#include <coreplugin/ProjectWindowInterface.h>

#include <visualeditor/ProjectViewModelContext.h>

namespace VisualEditor {
    ProjectAddOn::ProjectAddOn(QObject *parent) : WindowInterfaceAddOn(parent) {
    }

    ProjectAddOn::~ProjectAddOn() = default;

    void ProjectAddOn::initialize() {
        new ProjectViewModelContext(windowHandle()->cast<Core::ProjectWindowInterface>());
    }

    void ProjectAddOn::extensionsInitialized() {
    }

    bool ProjectAddOn::delayedInitialize() {
        return WindowInterfaceAddOn::delayedInitialize();
    }
}