#ifndef DIFFSCOPE_VISUAL_EDITOR_ARRANGEMENTADDON_H
#define DIFFSCOPE_VISUAL_EDITOR_ARRANGEMENTADDON_H

#include <CoreApi/windowinterface.h>

namespace VisualEditor {

    class ArrangementAddOn : public Core::WindowInterfaceAddOn {
        Q_OBJECT
    public:
        explicit ArrangementAddOn(QObject *parent = nullptr);
        ~ArrangementAddOn() override;

        void initialize() override;
        void extensionsInitialized() override;
        bool delayedInitialize() override;
    };

}

#endif //DIFFSCOPE_VISUAL_EDITOR_ARRANGEMENTADDON_H
