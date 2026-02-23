#ifndef DIFFSCOPE_VISUAL_EDITOR_SCROLLADDON_H
#define DIFFSCOPE_VISUAL_EDITOR_SCROLLADDON_H

#include <CoreApi/windowinterface.h>

namespace VisualEditor::Internal {

    class ScrollAddOn : public Core::WindowInterfaceAddOn {
        Q_OBJECT
        Q_PROPERTY(ActiveEditingArea activeEditingArea READ activeEditingArea WRITE setActiveEditingArea NOTIFY activeEditingAreaChanged)
    public:
        explicit ScrollAddOn(QObject *parent = nullptr);
        ~ScrollAddOn() override;

        void initialize() override;
        void extensionsInitialized() override;
        bool delayedInitialize() override;

        enum ActiveEditingArea {
            Arrangement,
            PianoRoll,
            Mixer,
        };
        Q_ENUM(ActiveEditingArea)
        ActiveEditingArea activeEditingArea() const;
        void setActiveEditingArea(ActiveEditingArea area);

    Q_SIGNALS:
        void activeEditingAreaChanged();

    private:
        QObject *m_actionsObject{};
        ActiveEditingArea m_activeEditingArea{Arrangement};
    };

}

#endif //DIFFSCOPE_VISUAL_EDITOR_SCROLLADDON_H
