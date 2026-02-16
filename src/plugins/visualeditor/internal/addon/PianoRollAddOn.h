#ifndef DIFFSCOPE_VISUAL_EDITOR_PIANOROLLADDON_H
#define DIFFSCOPE_VISUAL_EDITOR_PIANOROLLADDON_H

#include <CoreApi/windowinterface.h>

namespace VisualEditor {
    class PianoRollPanelInterface;
}

namespace VisualEditor::Internal {

    class AdditionalTrackLoader;

    class PianoRollAddOn : public Core::WindowInterfaceAddOn {
        Q_OBJECT
        Q_PROPERTY(VisualEditor::PianoRollPanelInterface *pianoRollPanelInterface READ pianoRollPanelInterface CONSTANT)
        Q_PROPERTY(AdditionalTrackLoader *additionalTrackLoader READ additionalTrackLoader CONSTANT)
        Q_PROPERTY(bool altPressed READ altPressed NOTIFY altPressedChanged)
        Q_PROPERTY(bool trackSelectorVisible READ isTrackSelectorVisible WRITE setTrackSelectorVisible NOTIFY trackSelectorVisibleChanged)
    public:
        explicit PianoRollAddOn(QObject *parent = nullptr);
        ~PianoRollAddOn() override;

        void initialize() override;
        void extensionsInitialized() override;
        bool delayedInitialize() override;

        PianoRollPanelInterface *pianoRollPanelInterface() const;
    AdditionalTrackLoader *additionalTrackLoader() const;

        bool eventFilter(QObject *watched, QEvent *event) override;

        bool altPressed() const;

        bool isTrackSelectorVisible() const;
        void setTrackSelectorVisible(bool visible);

    Q_SIGNALS:
        void altPressedChanged();
        void trackSelectorVisibleChanged();

    private:
        AdditionalTrackLoader *m_additionalTrackLoader{};
        bool m_altPressed{};
        bool m_trackSelectorVisible{};
    };

}

#endif //DIFFSCOPE_VISUAL_EDITOR_PIANOROLLADDON_H
