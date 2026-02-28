#ifndef DIFFSCOPE_VISUAL_EDITOR_KEYSIGNATURETRACKADDON_H
#define DIFFSCOPE_VISUAL_EDITOR_KEYSIGNATURETRACKADDON_H

#include <CoreApi/windowinterface.h>

namespace VisualEditor::Internal {

    class KeySignatureTrackAddOn : public Core::WindowInterfaceAddOn {
        Q_OBJECT
    public:
        explicit KeySignatureTrackAddOn(QObject *parent = nullptr);
        ~KeySignatureTrackAddOn() override;

        void initialize() override;
        void extensionsInitialized() override;
        bool delayedInitialize() override;
    };

}

#endif //DIFFSCOPE_VISUAL_EDITOR_KEYSIGNATURETRACKADDON_H
