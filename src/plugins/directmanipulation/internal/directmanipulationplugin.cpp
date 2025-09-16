#include "directmanipulationplugin.h"

#include <coreplugin/projectwindowinterface.h>

#include "directmanipulationaddon.h"

#include <QWDMHCore/DirectManipulationSystem.h>

namespace DirectManipulation::Internal {

    DirectManipulationPlugin::DirectManipulationPlugin() = default;

    DirectManipulationPlugin::~DirectManipulationPlugin() = default;

    bool DirectManipulationPlugin::initialize(const QStringList &arguments, QString *errorMessage) {
        new QWDMH::DirectManipulationSystem(this);
        Core::ProjectWindowInterfaceRegistry::instance()->attach<DirectManipulationAddOn>();
        return true;
    }

    void DirectManipulationPlugin::extensionsInitialized() {
    }

}
