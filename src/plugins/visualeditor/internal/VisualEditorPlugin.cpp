#include "VisualEditorPlugin.h"

#include <QSettings>

#include <CoreApi/runtimeinterface.h>
#include <CoreApi/settingcatalog.h>
#include <CoreApi/translationmanager.h>

#include <extensionsystem/pluginspec.h>

#include <QAKCore/actionregistry.h>

#include <coreplugin/CoreInterface.h>
#include <coreplugin/HomeWindowInterface.h>
#include <coreplugin/ProjectWindowInterface.h>

#include <visualeditor/internal/ArrangementAddOn.h>
#include <visualeditor/internal/PianoRollAddOn.h>
#include <visualeditor/internal/EditorPreference.h>
#include <visualeditor/internal/ProjectAddOn.h>
#include <visualeditor/internal/LabelTrackAddOn.h>
#include <visualeditor/internal/TempoTrackAddOn.h>
#include <visualeditor/internal/MixerAddOn.h>
#include <visualeditor/internal/EditorPage.h>

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

        initializeSingletons();
        initializeEditorPreference();
        initializeSettings();
        initializeWindows();

        return true;
    }

    void VisualEditorPlugin::extensionsInitialized() {
    }

    bool VisualEditorPlugin::delayedInitialize() {
        return IPlugin::delayedInitialize();
    }

    void VisualEditorPlugin::initializeSingletons() {
        new EditorPreference(this);
    }

    void VisualEditorPlugin::initializeEditorPreference() {
        auto editorPreference = EditorPreference::instance();
        editorPreference->load();
    }

    void VisualEditorPlugin::initializeSettings() {
        auto sc = Core::CoreInterface::settingCatalog();
        sc->addPage(new EditorPage);
    }

    void VisualEditorPlugin::initializeWindows() {
        Core::ProjectWindowInterfaceRegistry::instance()->attach<ProjectAddOn>();
        Core::ProjectWindowInterfaceRegistry::instance()->attach<ArrangementAddOn>();
        Core::ProjectWindowInterfaceRegistry::instance()->attach<PianoRollAddOn>();
        Core::ProjectWindowInterfaceRegistry::instance()->attach<MixerAddOn>();
        Core::ProjectWindowInterfaceRegistry::instance()->attach<LabelTrackAddOn>();
        Core::ProjectWindowInterfaceRegistry::instance()->attach<TempoTrackAddOn>();
    }
}
