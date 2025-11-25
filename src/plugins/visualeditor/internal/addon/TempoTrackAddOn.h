#ifndef DIFFSCOPE_VISUAL_EDITOR_TEMPOTRACKADDON_H
#define DIFFSCOPE_VISUAL_EDITOR_TEMPOTRACKADDON_H

#include <CoreApi/windowinterface.h>

namespace VisualEditor::Internal {

    class TempoTrackAddOn : public Core::WindowInterfaceAddOn {
        Q_OBJECT
    public:
        explicit TempoTrackAddOn(QObject *parent = nullptr);
        ~TempoTrackAddOn() override;

        void initialize() override;
        void extensionsInitialized() override;
        bool delayedInitialize() override;
    };

}

#endif //DIFFSCOPE_VISUAL_EDITOR_TEMPOTRACKADDON_H