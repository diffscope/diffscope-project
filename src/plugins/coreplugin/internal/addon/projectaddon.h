#ifndef DIFFSCOPE_COREPLUGIN_PROJECTADDON_H
#define DIFFSCOPE_COREPLUGIN_PROJECTADDON_H

#include <CoreApi/iwindow.h>

namespace Core::Internal {

    class ProjectAddon : public IWindowAddOn {
        Q_OBJECT
    public:
        explicit ProjectAddon(QObject *parent = nullptr);
        ~ProjectAddon() override;

        void initialize() override;
        void extensionsInitialized() override;
        bool delayedInitialize() override;
    };

}

#endif //DIFFSCOPE_COREPLUGIN_PROJECTADDON_H
