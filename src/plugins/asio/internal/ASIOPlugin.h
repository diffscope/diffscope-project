// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: GPL-3.0-only

#ifndef DIFFSCOPE_ASIO_ASIOPLUGIN_H
#define DIFFSCOPE_ASIO_ASIOPLUGIN_H

#include <extensionsystem/iplugin.h>

namespace ASIO::Internal {

    class ASIOPlugin : public ExtensionSystem::IPlugin {
        Q_OBJECT
        Q_PLUGIN_METADATA(IID "org.OpenVPI.DiffScope.Plugin" FILE "plugin.json")

    public:
        bool initialize(const QStringList &arguments, QString *errorMessage) override;
        void extensionsInitialized() override;
    };

}

#endif // DIFFSCOPE_ASIO_ASIOPLUGIN_H
