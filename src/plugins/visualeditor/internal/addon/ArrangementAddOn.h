#ifndef DIFFSCOPE_VISUAL_EDITOR_ARRANGEMENTADDON_H
#define DIFFSCOPE_VISUAL_EDITOR_ARRANGEMENTADDON_H

#include <CoreApi/windowinterface.h>

namespace VisualEditor {
    class ArrangementPanelInterface;
}

namespace VisualEditor::Internal {

    class AdditionalTrackLoader;

    class ArrangementAddOn : public Core::WindowInterfaceAddOn {
        Q_OBJECT
        Q_PROPERTY(VisualEditor::ArrangementPanelInterface *arrangementPanelInterface READ arrangementPanelInterface CONSTANT)
        Q_PROPERTY(AdditionalTrackLoader *additionalTrackLoader READ additionalTrackLoader CONSTANT)
        Q_PROPERTY(bool altPressed READ altPressed NOTIFY altPressedChanged)
    public:
        explicit ArrangementAddOn(QObject *parent = nullptr);
        ~ArrangementAddOn() override;

        void initialize() override;
        void extensionsInitialized() override;
        bool delayedInitialize() override;

        ArrangementPanelInterface *arrangementPanelInterface() const;

        AdditionalTrackLoader *additionalTrackLoader() const;

        bool eventFilter(QObject *watched, QEvent *event) override;

        bool altPressed() const;

    Q_SIGNALS:
        void altPressedChanged();

    private:
        AdditionalTrackLoader *m_additionalTrackLoader{};
        bool m_altPressed{};

    };

}

#endif //DIFFSCOPE_VISUAL_EDITOR_ARRANGEMENTADDON_H
