#ifndef DIFFSCOPE_VISUAL_EDITOR_PIANOROLLADDON_H
#define DIFFSCOPE_VISUAL_EDITOR_PIANOROLLADDON_H

#include <CoreApi/windowinterface.h>

namespace VisualEditor {
    class PianoRollPanelInterface;
}

namespace VisualEditor::Internal {

    class PianoRollAddOn : public Core::WindowInterfaceAddOn {
        Q_OBJECT
        Q_PROPERTY(VisualEditor::PianoRollPanelInterface *pianoRollPanelInterface READ pianoRollPanelInterface CONSTANT)
        Q_PROPERTY(bool altPressed READ altPressed NOTIFY altPressedChanged)
    public:
        explicit PianoRollAddOn(QObject *parent = nullptr);
        ~PianoRollAddOn() override;

        void initialize() override;
        void extensionsInitialized() override;
        bool delayedInitialize() override;

        PianoRollPanelInterface *pianoRollPanelInterface() const;

        bool eventFilter(QObject *watched, QEvent *event) override;

        bool altPressed() const;

    Q_SIGNALS:
        void altPressedChanged();

    private:
        bool m_altPressed{};
    };

}

#endif //DIFFSCOPE_VISUAL_EDITOR_PIANOROLLADDON_H
