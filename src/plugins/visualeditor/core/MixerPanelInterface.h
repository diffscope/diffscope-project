#ifndef DIFFSCOPE_VISUALEDITOR_MIXERPANELINTERFACE_H
#define DIFFSCOPE_VISUALEDITOR_MIXERPANELINTERFACE_H

#include <QObject>
#include <qqmlintegration.h>

#include <visualeditor/visualeditorglobal.h>

class QQuickItem;

namespace sflow {
    class ScrollBehaviorViewModel;
    class TrackListLayoutViewModel;
    class TrackListInteractionController;
}

namespace Core {
    class ProjectWindowInterface;
}

namespace VisualEditor {

    namespace Internal {
        class MixerAddOn;
    }

    class MixerPanelInterfacePrivate;

    class VISUAL_EDITOR_EXPORT MixerPanelInterface : public QObject {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_DECLARE_PRIVATE(MixerPanelInterface)
        Q_PROPERTY(Core::ProjectWindowInterface *windowHandle READ windowHandle CONSTANT)
        Q_PROPERTY(sflow::TrackListLayoutViewModel *trackListLayoutViewModel READ trackListLayoutViewModel CONSTANT)
        Q_PROPERTY(sflow::TrackListLayoutViewModel *masterTrackListLayoutViewModel READ masterTrackListLayoutViewModel CONSTANT)
        Q_PROPERTY(sflow::ScrollBehaviorViewModel *scrollBehaviorViewModel READ scrollBehaviorViewModel CONSTANT)
        Q_PROPERTY(sflow::TrackListInteractionController *trackListInteractionController READ trackListInteractionController CONSTANT)
        Q_PROPERTY(sflow::TrackListInteractionController *masterTrackListInteractionController READ masterTrackListInteractionController CONSTANT)
        Q_PROPERTY(QQuickItem *mixerView READ mixerView CONSTANT)
        Q_PROPERTY(Tool tool READ tool WRITE setTool NOTIFY toolChanged)

    public:
        ~MixerPanelInterface() override;

        static MixerPanelInterface *of(const Core::ProjectWindowInterface *windowHandle);

        Core::ProjectWindowInterface *windowHandle() const;

        sflow::TrackListLayoutViewModel *trackListLayoutViewModel() const;
        sflow::TrackListLayoutViewModel *masterTrackListLayoutViewModel() const;
        sflow::ScrollBehaviorViewModel *scrollBehaviorViewModel() const;
        sflow::TrackListInteractionController *trackListInteractionController() const;
        sflow::TrackListInteractionController *masterTrackListInteractionController() const;

        QQuickItem *mixerView() const;

        enum Tool {
            PointerTool,
            HandTool,
        };
        Q_ENUM(Tool)
        Tool tool() const;
        void setTool(Tool tool);

    Q_SIGNALS:
        void toolChanged();

    private:
        friend class Internal::MixerAddOn;
        explicit MixerPanelInterface(Internal::MixerAddOn *addOn, Core::ProjectWindowInterface *windowHandle);
        QScopedPointer<MixerPanelInterfacePrivate> d_ptr;
    };

}

#endif //DIFFSCOPE_VISUALEDITOR_MIXERPANELINTERFACE_H
