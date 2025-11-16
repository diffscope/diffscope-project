#ifndef DIFFSCOPE_VISUAL_EDITOR_PROJECTADDON_H
#define DIFFSCOPE_VISUAL_EDITOR_PROJECTADDON_H

#include <CoreApi/windowinterface.h>

namespace VisualEditor::Internal {

    class ProjectAddOn : public Core::WindowInterfaceAddOn {
        Q_OBJECT
    public:
        explicit ProjectAddOn(QObject *parent = nullptr);
        ~ProjectAddOn() override;

        void initialize() override;
        void extensionsInitialized() override;
        bool delayedInitialize() override;
    };

}

#endif //DIFFSCOPE_VISUAL_EDITOR_PROJECTADDON_H
