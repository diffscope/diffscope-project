#ifndef DIFFSCOPE_COREPLUGIN_HOMEADDON_H
#define DIFFSCOPE_COREPLUGIN_HOMEADDON_H

#include <CoreApi/windowinterface.h>

namespace Core::Internal {

    class HomeAddOn : public WindowInterfaceAddOn {
        Q_OBJECT
    public:
        explicit HomeAddOn(QObject *parent = nullptr);
        ~HomeAddOn() override;

        void initialize() override;
        void extensionsInitialized() override;
        bool delayedInitialize() override;
    };

}

#endif //DIFFSCOPE_COREPLUGIN_HOMEADDON_H
