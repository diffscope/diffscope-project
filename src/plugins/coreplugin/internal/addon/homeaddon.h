#ifndef DIFFSCOPE_COREPLUGIN_HOMEADDON_H
#define DIFFSCOPE_COREPLUGIN_HOMEADDON_H

#include <CoreApi/iwindow.h>

namespace Core::Internal {

    class HomeAddOn : public IWindowAddOn {
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
