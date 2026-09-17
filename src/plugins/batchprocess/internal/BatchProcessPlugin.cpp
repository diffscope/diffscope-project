// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "BatchProcessPlugin.h"

#include <QLoggingCategory>

#include <CoreApi/runtimeinterface.h>
#include <CoreApi/settingcatalog.h>
#include <CoreApi/translationmanager.h>

#include <extensionsystem/pluginspec.h>

#include <QAKCore/actionregistry.h>

#include <coreplugin/CoreInterface.h>
#include <coreplugin/HomeWindowInterface.h>
#include <coreplugin/ProjectWindowInterface.h>

#include <batchprocess/BatchProcessInterface.h>
#include <batchprocess/FileSystemAccessInterface.h>
#include <batchprocess/JavaScriptConsoleInterface.h>
#include <batchprocess/ShellInterface.h>
#include <batchprocess/internal/BatchProcessAddOn.h>
#include <batchprocess/internal/BatchProcessPage.h>
#include <batchprocess/internal/BatchProcessSettings.h>
#include <batchprocess/internal/FileSystemModuleExtension.h>
#include <batchprocess/internal/JavaScriptConsoleAddOn.h>
#include <batchprocess/internal/JavaScriptConsoleExtension.h>
#include <batchprocess/internal/RuntimeModuleExtension.h>
#include <batchprocess/internal/ShellModuleExtension.h>

static auto getBatchProcessActionExtension() {
    return QAK_STATIC_ACTION_EXTENSION(batchprocess);
}

namespace BatchProcess::Internal {

    Q_STATIC_LOGGING_CATEGORY(lcBatchProcessPlugin, "diffscope.batchprocess.plugin")

    BatchProcessPlugin::BatchProcessPlugin() = default;

    BatchProcessPlugin::~BatchProcessPlugin() = default;

    bool BatchProcessPlugin::initialize(const QStringList &arguments, QString *errorMessage) {
        Q_UNUSED(arguments)
        Q_UNUSED(errorMessage)

        qCInfo(lcBatchProcessPlugin) << "Initializing Batch Process plugin";
        Core::RuntimeInterface::translationManager()->addTranslationPath(pluginSpec()->location() + QStringLiteral("/translations"));

        new BatchProcessSettings(this);
        auto batchProcessInterface = new BatchProcessInterface(this);
        auto consoleInterface = new JavaScriptConsoleInterface(this);
        auto fileSystemAccessInterface = new FileSystemAccessInterface(this);
        auto shellInterface = new ShellInterface(this);
        new JavaScriptConsoleExtension(batchProcessInterface, consoleInterface, this);
        new FileSystemModuleExtension(batchProcessInterface, fileSystemAccessInterface, this);
        new ShellModuleExtension(batchProcessInterface, shellInterface, fileSystemAccessInterface, this);
        new RuntimeModuleExtension(batchProcessInterface, this);

        Core::CoreInterface::actionRegistry()->addExtension(::getBatchProcessActionExtension());
        Core::HomeWindowInterfaceRegistry::instance()->attach<BatchProcessAddOn>();
        Core::HomeWindowInterfaceRegistry::instance()->attach<JavaScriptConsoleAddOn>();
        Core::ProjectWindowInterfaceRegistry::instance()->attach<BatchProcessAddOn>();
        Core::ProjectWindowInterfaceRegistry::instance()->attach<JavaScriptConsoleAddOn>();
        Core::CoreInterface::settingCatalog()->addPage(new BatchProcessPage);
        return true;
    }

    void BatchProcessPlugin::extensionsInitialized() {
        qCInfo(lcBatchProcessPlugin) << "Initializing Batch Process script runtime";
        BatchProcessInterface::instance()->reloadScripts();
    }

    bool BatchProcessPlugin::delayedInitialize() {
        return IPlugin::delayedInitialize();
    }

}

#include "moc_BatchProcessPlugin.cpp"
