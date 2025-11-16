#ifndef DIFFSCOPE_VISUAL_EDITOR_ARRANGEMENTADDON_H
#define DIFFSCOPE_VISUAL_EDITOR_ARRANGEMENTADDON_H

#include <CoreApi/windowinterface.h>

namespace VisualEditor {
    class ArrangementPanelInterface;
}

namespace VisualEditor::Internal {

    class ArrangementAddOn : public Core::WindowInterfaceAddOn {
        Q_OBJECT
        Q_PROPERTY(VisualEditor::ArrangementPanelInterface *arrangementPanelInterface READ arrangementPanelInterface CONSTANT)
    public:
        explicit ArrangementAddOn(QObject *parent = nullptr);
        ~ArrangementAddOn() override;

        void initialize() override;
        void extensionsInitialized() override;
        bool delayedInitialize() override;

        ArrangementPanelInterface *arrangementPanelInterface() const;

        bool eventFilter(QObject *watched, QEvent *event) override;

    };

}

#endif //DIFFSCOPE_VISUAL_EDITOR_ARRANGEMENTADDON_H
