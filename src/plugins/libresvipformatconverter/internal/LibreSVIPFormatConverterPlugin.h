#ifndef LIBRESVIP_FORMAT_CONVERTER_LIBRESVIPFORMATCONVERTERPLUGIN_H
#define LIBRESVIP_FORMAT_CONVERTER_LIBRESVIPFORMATCONVERTERPLUGIN_H

#include <extensionsystem/iplugin.h>

namespace LibreSVIPFormatConverter::Internal {

    class LibreSVIPFormatConverterPlugin : public ExtensionSystem::IPlugin {
        Q_OBJECT
        Q_PLUGIN_METADATA(IID "org.OpenVPI.DiffScope.Plugin" FILE "plugin.json")
    public:
        LibreSVIPFormatConverterPlugin();
        ~LibreSVIPFormatConverterPlugin() override;

        bool initialize(const QStringList &arguments, QString *errorMessage) override;
        void extensionsInitialized() override;
        bool delayedInitialize() override;
    };


}

#endif //LIBRESVIP_FORMAT_CONVERTER_LIBRESVIPFORMATCONVERTERPLUGIN_H
