// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_BATCHPROCESS_BATCHPROCESSPLUGIN_H
#define DIFFSCOPE_BATCHPROCESS_BATCHPROCESSPLUGIN_H

#include <extensionsystem/iplugin.h>

namespace BatchProcess::Internal {

    class BatchProcessPlugin : public ExtensionSystem::IPlugin {
        Q_OBJECT
        Q_PLUGIN_METADATA(IID "org.OpenVPI.DiffScope.Plugin" FILE "plugin.json")
    public:
        BatchProcessPlugin();
        ~BatchProcessPlugin() override;

        bool initialize(const QStringList &arguments, QString *errorMessage) override;
        void extensionsInitialized() override;
        bool delayedInitialize() override;
    };

}

#endif // DIFFSCOPE_BATCHPROCESS_BATCHPROCESSPLUGIN_H
