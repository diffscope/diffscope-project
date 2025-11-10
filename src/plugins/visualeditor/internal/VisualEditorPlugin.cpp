#include "VisualEditorPlugin.h"

#include <QSettings>

#include <CoreApi/runtimeinterface.h>
#include <CoreApi/translationmanager.h>

#include <extensionsystem/pluginspec.h>

#include <QAKCore/actionregistry.h>

#include <coreplugin/CoreInterface.h>
#include <coreplugin/HomeWindowInterface.h>
#include <coreplugin/ProjectWindowInterface.h>

#include <visualeditor/internal/ProjectAddOn.h>
#include <visualeditor/internal/ArrangementAddOn.h>

static auto getVisualEditorActionExtension() {
    return QAK_STATIC_ACTION_EXTENSION(visualeditor);
}

namespace VisualEditor::Internal {

    VisualEditorPlugin::VisualEditorPlugin() {
    }

    VisualEditorPlugin::~VisualEditorPlugin() = default;

    bool VisualEditorPlugin::initialize(const QStringList &arguments, QString *errorMessage) {
        Core::RuntimeInterface::translationManager()->addTranslationPath(pluginSpec()->location() + QStringLiteral("/translations"));
        Core::CoreInterface::actionRegistry()->addExtension(::getVisualEditorActionExtension());

        Core::ProjectWindowInterfaceRegistry::instance()->attach<ProjectAddOn>();
        Core::ProjectWindowInterfaceRegistry::instance()->attach<ArrangementAddOn>();

        return true;
    }
    void VisualEditorPlugin::extensionsInitialized() {
    }
    bool VisualEditorPlugin::delayedInitialize() {
        return IPlugin::delayedInitialize();
    }
}
