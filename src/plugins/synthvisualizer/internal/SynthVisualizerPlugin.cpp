// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "SynthVisualizerPlugin.h"

#include <QQmlComponent>

#include <CoreApi/runtimeinterface.h>
#include <CoreApi/settingcatalog.h>
#include <CoreApi/translationmanager.h>

#include <extensionsystem/pluginspec.h>

#include <QAKCore/actionregistry.h>

#include <coreplugin/CoreInterface.h>
#include <coreplugin/ProjectWindowInterface.h>

#include <visualeditor/ViewLayerRegistry.h>
#include <synthvisualizer/internal/SynthesisPieceTrackAddOn.h>
#include <synthvisualizer/internal/SynthesisVisualizationPage.h>
#include <synthvisualizer/internal/SynthesisVisualizerSettings.h>

#include <org.diffscope.synthvisualizer_actions.qak.h>

namespace SynthVisualizer::Internal {

    SynthVisualizerPlugin::SynthVisualizerPlugin() = default;
    SynthVisualizerPlugin::~SynthVisualizerPlugin() = default;

    bool SynthVisualizerPlugin::initialize(const QStringList &, QString *) {
        new SynthesisVisualizerSettings(this);
        Core::RuntimeInterface::translationManager()->addTranslationPath(
            pluginSpec()->location() + QStringLiteral("/translations")
        );
        Core::CoreInterface::actionRegistry()->addExtension(::synthvisualizerActionExtension());
        Core::ProjectWindowInterfaceRegistry::instance()->attach<SynthesisPieceTrackAddOn>();
        return true;
    }

    void SynthVisualizerPlugin::extensionsInitialized() {
        using VisualEditor::ViewLayerPoint;
        auto registry = VisualEditor::ViewLayerRegistry::instance();
        const auto registerLayer = [this, registry](ViewLayerPoint point, QAnyStringView typeName) {
            auto component = new QQmlComponent(Core::RuntimeInterface::qmlEngine(), "DiffScope.SynthVisualizer", typeName, this);
            if (component->isError())
                qFatal() << component->errorString();
            registry->registerLayer(point, component);
        };

        registerLayer(ViewLayerPoint::PhonemeBackground, "SynthesisWaveformBackground");
        registerLayer(ViewLayerPoint::ArrangementClipAdornment, "SingingClipWaveformAdornment");

        auto catalog = Core::CoreInterface::settingCatalog();
        auto page = new SynthesisVisualizationPage;
        if (auto editorPage = catalog->page(QStringLiteral("org.diffscope.visualeditor.Editor")))
            editorPage->addPage(page);
        else
            catalog->addPage(page);
    }

    bool SynthVisualizerPlugin::delayedInitialize() {
        return IPlugin::delayedInitialize();
    }

}
