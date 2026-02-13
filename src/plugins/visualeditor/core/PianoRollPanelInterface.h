#ifndef DIFFSCOPE_VISUALEDITOR_PIANOROLLPANELINTERFACE_H
#define DIFFSCOPE_VISUALEDITOR_PIANOROLLPANELINTERFACE_H

#include <QObject>
#include <qqmlintegration.h>

#include <visualeditor/visualeditorglobal.h>

class QQuickItem;

namespace sflow {
    class TimeViewModel;
    class TimeLayoutViewModel;
    class ScrollBehaviorViewModel;
    class TimelineInteractionController;
    class ClavierViewModel;
    class ClavierInteractionController;
}

namespace Core {
    class ProjectWindowInterface;
}

namespace VisualEditor {

    namespace Internal {
        class PianoRollAddOn;
    }

    class PositionAlignmentManipulator;
    class AutoPageScrollingManipulator;

    class PianoRollPanelInterfacePrivate;

    class VISUAL_EDITOR_EXPORT PianoRollPanelInterface : public QObject {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_DECLARE_PRIVATE(PianoRollPanelInterface)
        Q_PROPERTY(Core::ProjectWindowInterface *windowHandle READ windowHandle CONSTANT)
        Q_PROPERTY(sflow::TimeViewModel *timeViewModel READ timeViewModel CONSTANT)
        Q_PROPERTY(sflow::TimeLayoutViewModel *timeLayoutViewModel READ timeLayoutViewModel CONSTANT)
        Q_PROPERTY(sflow::ScrollBehaviorViewModel *scrollBehaviorViewModel READ scrollBehaviorViewModel CONSTANT)
        Q_PROPERTY(sflow::TimelineInteractionController *timelineInteractionController READ timelineInteractionController CONSTANT)
        Q_PROPERTY(sflow::ClavierViewModel *clavierViewModel READ clavierViewModel CONSTANT)
        Q_PROPERTY(sflow::ClavierInteractionController *clavierInteractionController READ clavierInteractionController CONSTANT)
        Q_PROPERTY(PositionAlignmentManipulator *positionAlignmentManipulator READ positionAlignmentManipulator CONSTANT)
        Q_PROPERTY(AutoPageScrollingManipulator *autoPageScrollingManipulator READ autoPageScrollingManipulator CONSTANT)
        Q_PROPERTY(QQuickItem *pianoRollView READ pianoRollView CONSTANT)
        Q_PROPERTY(Tool tool READ tool WRITE setTool NOTIFY toolChanged)
        Q_PROPERTY(bool snapTemporarilyDisabled READ isSnapTemporarilyDisabled WRITE setSnapTemporarilyDisabled NOTIFY snapTemporarilyDisabledChanged)
        Q_PROPERTY(bool mouseTrackingDisabled READ isMouseTrackingDisabled WRITE setMouseTrackingDisabled NOTIFY mouseTrackingDisabledChanged)

    public:
        ~PianoRollPanelInterface() override;

        static PianoRollPanelInterface *of(const Core::ProjectWindowInterface *windowHandle);

        Core::ProjectWindowInterface *windowHandle() const;

        sflow::TimeViewModel *timeViewModel() const;
        sflow::TimeLayoutViewModel *timeLayoutViewModel() const;
        sflow::ScrollBehaviorViewModel *scrollBehaviorViewModel() const;
        sflow::TimelineInteractionController *timelineInteractionController() const;
        sflow::ClavierViewModel *clavierViewModel() const;
        sflow::ClavierInteractionController *clavierInteractionController() const;

        PositionAlignmentManipulator *positionAlignmentManipulator() const;
        AutoPageScrollingManipulator *autoPageScrollingManipulator() const;

        QQuickItem *pianoRollView() const;

        enum Tool {
            PointerTool,
            PencilTool,
            ScissorTool,
            SelectTool,
            HandTool,
        };
        Q_ENUM(Tool)
        Tool tool() const;
        void setTool(Tool tool);

        bool isSnapTemporarilyDisabled() const;
        void setSnapTemporarilyDisabled(bool disabled);

        bool isMouseTrackingDisabled() const;
        void setMouseTrackingDisabled(bool disabled);

    Q_SIGNALS:
        void toolChanged();
        void snapTemporarilyDisabledChanged();
        void mouseTrackingDisabledChanged();

    private:
        friend class Internal::PianoRollAddOn;
        explicit PianoRollPanelInterface(Internal::PianoRollAddOn *addOn, Core::ProjectWindowInterface *windowHandle);
        QScopedPointer<PianoRollPanelInterfacePrivate> d_ptr;
    };

}

#endif //DIFFSCOPE_VISUALEDITOR_PIANOROLLPANELINTERFACE_H
