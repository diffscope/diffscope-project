#ifndef DIFFSCOPE_VISUAL_EDITOR_ADDITIONALTRACKADDON_H
#define DIFFSCOPE_VISUAL_EDITOR_ADDITIONALTRACKADDON_H

#include <CoreApi/windowinterface.h>

namespace VisualEditor::Internal {

    class AdditionalTrackAddOn : public Core::WindowInterfaceAddOn {
        Q_OBJECT
    public:
        explicit AdditionalTrackAddOn(QObject *parent = nullptr);
        ~AdditionalTrackAddOn() override;

        void initialize() override;
        void extensionsInitialized() override;
        bool delayedInitialize() override;
    };

}

#endif //DIFFSCOPE_VISUAL_EDITOR_ADDITIONALTRACKADDON_H
