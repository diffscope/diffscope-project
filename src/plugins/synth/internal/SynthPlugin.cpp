// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "SynthPlugin.h"

#include <QLoggingCategory>
#include <QSplashScreen>

#include <CoreApi/runtimeinterface.h>
#include <CoreApi/settingcatalog.h>
#include <CoreApi/translationmanager.h>

#include <extensionsystem/pluginspec.h>

#include <QAKCore/actionregistry.h>

#include <coreplugin/CoreInterface.h>
#include <coreplugin/ProjectWindowInterface.h>

#include <synth/internal/ArchitecturePage.h>
#include <synth/internal/ParametersPage.h>
#include <synth/internal/ServicesPage.h>
#include <synth/internal/SynthService.h>
#include <synth/internal/SynthesisPage.h>
#include <synth/internal/SynthesisProjectAddOn.h>
#include <synth/internal/SynthesisServicePanelAddOn.h>

static auto synthActionExtension() {
    return QAK_STATIC_ACTION_EXTENSION(synth);
}

namespace Synth::Internal {

    Q_STATIC_LOGGING_CATEGORY(lcSynthPlugin, "diffscope.synth.plugin")

    SynthPlugin::SynthPlugin() = default;
    SynthPlugin::~SynthPlugin() = default;

    bool SynthPlugin::initialize(const QStringList &, QString *errorMessage) {
        Core::RuntimeInterface::translationManager()->addTranslationPath(
            pluginSpec()->location() + QStringLiteral("/translations")
        );
        Core::CoreInterface::actionRegistry()->addExtension(::synthActionExtension());
        Core::RuntimeInterface::splash()->showMessage(tr("Initializing synthesis plugin..."));

        qCInfo(lcSynthPlugin) << "Initializing";
        m_service = new SynthService(this);
        if (!m_service->initialize(errorMessage)) {
            qCCritical(lcSynthPlugin) << "Initialization failed"
                                      << (errorMessage ? *errorMessage : QString{});
            return false;
        }

        auto rootPage = new SynthesisPage;
        rootPage->addPage(new ArchitecturePage);
        rootPage->addPage(new ServicesPage(m_service));
        rootPage->addPage(new ParametersPage(m_service));
        Core::CoreInterface::settingCatalog()->addPage(rootPage);

        Core::ProjectWindowInterfaceRegistry::instance()->attach<SynthesisServicePanelAddOn>();
        Core::ProjectWindowInterfaceRegistry::instance()->attach<SynthesisProjectAddOn>();
        qCInfo(lcSynthPlugin) << "Initialized";
        return true;
    }

    void SynthPlugin::extensionsInitialized() {
    }

    bool SynthPlugin::delayedInitialize() {
        qCInfo(lcSynthPlugin) << "Starting delayed initialization and initial DSSP refresh";
        m_service->startDelayedInitialization();
        return IPlugin::delayedInitialize();
    }

    ExtensionSystem::IPlugin::ShutdownFlag SynthPlugin::aboutToShutdown() {
        qCInfo(lcSynthPlugin) << "Shutting down";
        if (m_service)
            m_service->shutdown();
        qCInfo(lcSynthPlugin) << "Shut down";
        return SynchronousShutdown;
    }

}
