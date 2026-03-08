#ifndef DIFFSCOPE_VISUAL_EDITOR_MIXERADDON_H
#define DIFFSCOPE_VISUAL_EDITOR_MIXERADDON_H

#include <CoreApi/windowinterface.h>

namespace VisualEditor {
    class MixerPanelInterface;
}

namespace VisualEditor::Internal {

    class MixerAddOn : public Core::WindowInterfaceAddOn {
        Q_OBJECT
        Q_PROPERTY(VisualEditor::MixerPanelInterface *mixerPanelInterface READ mixerPanelInterface CONSTANT)
    public:
        explicit MixerAddOn(QObject *parent = nullptr);
        ~MixerAddOn() override;

        void initialize() override;
        void extensionsInitialized() override;
        bool delayedInitialize() override;

        MixerPanelInterface *mixerPanelInterface() const;
    };

}

#endif //DIFFSCOPE_VISUAL_EDITOR_MIXERADDON_H
