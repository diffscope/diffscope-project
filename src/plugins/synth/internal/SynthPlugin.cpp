// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "SynthPlugin.h"

#include <QLoggingCategory>
#include <QQmlComponent>
#include <QSplashScreen>

#include <CoreApi/runtimeinterface.h>
#include <CoreApi/settingcatalog.h>
#include <CoreApi/translationmanager.h>

#include <extensionsystem/pluginspec.h>

#include <QAKCore/actionregistry.h>
#include <QAKQuick/quickactioncontext.h>

#include <coreplugin/CoreInterface.h>
#include <coreplugin/HomeWindowInterface.h>
#include <coreplugin/ProjectWindowInterface.h>

#include <synth/SynthInterface.h>
#include <synth/internal/ArchitecturePage.h>
#include <synth/internal/AudioExtractionAddOn.h>
#include <synth/internal/DsspServerAddOn.h>
#include <synth/internal/DsspServerManager.h>
#include <synth/internal/DsspServerPage.h>
#include <synth/internal/ServicesPage.h>
#include <synth/internal/SynthService.h>
#include <synth/internal/SynthesisPage.h>
#include <synth/internal/SynthesisProjectAddOn.h>
#include <synth/internal/SynthesisServicePanelAddOn.h>

#include <org.diffscope.synth_actions.qak.h>

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
        initializeControlPanels();

        m_dsspServer = new DsspServerManager(pluginSpec()->location(), this);
        Core::RuntimeInterface::instance()->addObject(m_dsspServer);
        m_dsspServer->initialize();

        auto rootPage = new SynthesisPage;
        rootPage->addPage(new ArchitecturePage);
        rootPage->addPage(new ServicesPage(m_service));
        rootPage->addPage(new DsspServerPage(m_dsspServer));
        Core::CoreInterface::settingCatalog()->addPage(rootPage);

        Core::ProjectWindowInterfaceRegistry::instance()->attach<SynthesisServicePanelAddOn>();
        Core::ProjectWindowInterfaceRegistry::instance()->attach<SynthesisProjectAddOn>();
        Core::ProjectWindowInterfaceRegistry::instance()->attach<AudioExtractionAddOn>();
        Core::HomeWindowInterfaceRegistry::instance()->attach<DsspServerAddOn>();
        Core::ProjectWindowInterfaceRegistry::instance()->attach<DsspServerAddOn>();
        qCInfo(lcSynthPlugin) << "Initialized";
        return true;
    }

    void SynthPlugin::extensionsInitialized() {
        m_dsspServer->enableNotifications();
        auto component = new QQmlComponent(Core::RuntimeInterface::qmlEngine(), QStringLiteral("DiffScope.Synth"), QStringLiteral("SynthesisEnvironmentWelcomeWizardPage"), this);
        if (component->isError())
            qFatal() << component->errorString();
        Core::CoreInterface::actionContext()->addAction(QStringLiteral("org.diffscope.synth.welcomeWizard.synthesisEnvironment"), component);
    }

    void SynthPlugin::initializeControlPanels() {
        auto engine = Core::RuntimeInterface::qmlEngine();
        auto architecturePanel = new QQmlComponent(engine, QStringLiteral("DiffScope.Synth"), QStringLiteral("DiffSingerArchitectureControlPanel"), this);
        if (architecturePanel->isError())
            qFatal() << architecturePanel->errorString();
        auto singerPanel = new QQmlComponent(engine, QStringLiteral("DiffScope.Synth"), QStringLiteral("DiffSingerSingerControlPanel"), this);
        if (singerPanel->isError())
            qFatal() << singerPanel->errorString();
        if (!SynthInterface::instance()->registerBuiltInArchitecture(QStringLiteral("diffsinger"), architecturePanel, singerPanel))
            qCWarning(lcSynthPlugin) << "The built-in DiffSinger control panels could not be registered.";
    }

    bool SynthPlugin::delayedInitialize() {
        qCInfo(lcSynthPlugin) << "Starting delayed initialization and initial DSSP refresh";
        m_service->startDelayedInitialization();
        return IPlugin::delayedInitialize();
    }

    ExtensionSystem::IPlugin::ShutdownFlag SynthPlugin::aboutToShutdown() {
        qCInfo(lcSynthPlugin) << "Shutting down";
        if (m_dsspServer)
            m_dsspServer->shutdown();
        if (m_service)
            m_service->shutdown();
        qCInfo(lcSynthPlugin) << "Shut down";
        return SynchronousShutdown;
    }

}
