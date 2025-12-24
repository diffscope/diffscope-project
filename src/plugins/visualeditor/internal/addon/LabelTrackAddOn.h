#ifndef DIFFSCOPE_VISUAL_EDITOR_LABELTRACKADDON_H
#define DIFFSCOPE_VISUAL_EDITOR_LABELTRACKADDON_H

#include <CoreApi/windowinterface.h>

namespace VisualEditor::Internal {

    class LabelTrackAddOn : public Core::WindowInterfaceAddOn {
        Q_OBJECT
    public:
        explicit LabelTrackAddOn(QObject *parent = nullptr);
        ~LabelTrackAddOn() override;

        void initialize() override;
        void extensionsInitialized() override;
        bool delayedInitialize() override;
    };

}

#endif //DIFFSCOPE_VISUAL_EDITOR_LABELTRACKADDON_H