#include "SynthVisualizerPlugin.h"

#include <CoreApi/runtimeinterface.h>
#include <CoreApi/translationmanager.h>

#include <extensionsystem/pluginspec.h>

#include <QAKCore/actionregistry.h>

#include <coreplugin/CoreInterface.h>
#include <coreplugin/ProjectWindowInterface.h>

#include <synthvisualizer/internal/SynthesisPieceTrackAddOn.h>

static auto synthVisualizerActionExtension() {
    return QAK_STATIC_ACTION_EXTENSION(synthvisualizer);
}

namespace SynthVisualizer::Internal {

    SynthVisualizerPlugin::SynthVisualizerPlugin() = default;
    SynthVisualizerPlugin::~SynthVisualizerPlugin() = default;

    bool SynthVisualizerPlugin::initialize(const QStringList &, QString *) {
        Core::RuntimeInterface::translationManager()->addTranslationPath(
            pluginSpec()->location() + QStringLiteral("/translations")
        );
        Core::CoreInterface::actionRegistry()->addExtension(::synthVisualizerActionExtension());
        Core::ProjectWindowInterfaceRegistry::instance()->attach<SynthesisPieceTrackAddOn>();
        return true;
    }

    void SynthVisualizerPlugin::extensionsInitialized() {
    }

    bool SynthVisualizerPlugin::delayedInitialize() {
        return IPlugin::delayedInitialize();
    }

}
