#ifndef MIDI_FORMAT_CONVERTER_MIDIFORMATCONVERTERPLUGIN_H
#define MIDI_FORMAT_CONVERTER_MIDIFORMATCONVERTERPLUGIN_H

#include <extensionsystem/iplugin.h>

namespace MIDIFormatConverter::Internal {

    class MIDIFormatConverterPlugin : public ExtensionSystem::IPlugin {
        Q_OBJECT
        Q_PLUGIN_METADATA(IID "org.OpenVPI.DiffScope.Plugin" FILE "plugin.json")
    public:
        MIDIFormatConverterPlugin();
        ~MIDIFormatConverterPlugin() override;

        bool initialize(const QStringList &arguments, QString *errorMessage) override;
        void extensionsInitialized() override;
        bool delayedInitialize() override;
    };


}

#endif //MIDI_FORMAT_CONVERTER_MIDIFORMATCONVERTERPLUGIN_H
