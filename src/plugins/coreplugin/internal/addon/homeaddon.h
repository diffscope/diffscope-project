#ifndef DIFFSCOPE_COREPLUGIN_HOMEADDON_H
#define DIFFSCOPE_COREPLUGIN_HOMEADDON_H

#include <CoreApi/iwindow.h>

namespace Core::Internal {

    class HomeAddon : public IWindowAddOn {
        Q_OBJECT
    public:
        explicit HomeAddon(QObject *parent = nullptr);
        ~HomeAddon() override;

        void initialize() override;
        void extensionsInitialized() override;
        bool delayedInitialize() override;
    };

}

#endif //DIFFSCOPE_COREPLUGIN_HOMEADDON_H
