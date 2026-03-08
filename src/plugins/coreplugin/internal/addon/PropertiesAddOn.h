#ifndef DIFFSCOPE_COREPLUGIN_PROPERTIESADDON_H
#define DIFFSCOPE_COREPLUGIN_PROPERTIESADDON_H

#include <CoreApi/windowinterface.h>

namespace Core::Internal {

    class PropertiesAddOn : public WindowInterfaceAddOn {
        Q_OBJECT
    public:
        explicit PropertiesAddOn(QObject *parent = nullptr);
        ~PropertiesAddOn() override;

        void initialize() override;
        void extensionsInitialized() override;
        bool delayedInitialize() override;

    private:
    };

}

#endif //DIFFSCOPE_COREPLUGIN_PROPERTIESADDON_H
