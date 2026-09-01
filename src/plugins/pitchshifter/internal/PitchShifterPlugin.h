// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef DIFFSCOPE_PITCH_SHIFTER_PITCHSHIFTERPLUGIN_H
#define DIFFSCOPE_PITCH_SHIFTER_PITCHSHIFTERPLUGIN_H

#include <extensionsystem/iplugin.h>

namespace PitchShifter::Internal {

    class PitchShifterPlugin : public ExtensionSystem::IPlugin {
        Q_OBJECT
        Q_PLUGIN_METADATA(IID "org.OpenVPI.DiffScope.Plugin" FILE "plugin.json")

    public:
        PitchShifterPlugin();
        ~PitchShifterPlugin() override;

        bool initialize(const QStringList &arguments, QString *errorMessage) override;
        void extensionsInitialized() override;
        bool delayedInitialize() override;
    };

}

#endif // DIFFSCOPE_PITCH_SHIFTER_PITCHSHIFTERPLUGIN_H
