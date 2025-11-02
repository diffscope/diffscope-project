#ifndef DIFFSCOPE_COREPLUGIN_METADATAADDON_H
#define DIFFSCOPE_COREPLUGIN_METADATAADDON_H

#include <CoreApi/windowinterface.h>

namespace Core::Internal {

    class MetadataAddOn : public WindowInterfaceAddOn {
        Q_OBJECT
    public:
        explicit MetadataAddOn(QObject *parent = nullptr);
        ~MetadataAddOn() override;

        void initialize() override;
        void extensionsInitialized() override;
        bool delayedInitialize() override;
    };

}

#endif //DIFFSCOPE_COREPLUGIN_METADATAADDON_H
