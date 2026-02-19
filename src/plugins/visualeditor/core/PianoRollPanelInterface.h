#ifndef DIFFSCOPE_VISUALEDITOR_PIANOROLLPANELINTERFACE_H
#define DIFFSCOPE_VISUALEDITOR_PIANOROLLPANELINTERFACE_H

#include <QObject>
#include <QAbstractItemModel>
#include <qqmlintegration.h>

#include <visualeditor/visualeditorglobal.h>

class QQuickItem;
class QAbstractItemModel;

namespace sflow {
    class TimeViewModel;
    class TimeLayoutViewModel;
    class ScrollBehaviorViewModel;
    class TimelineInteractionController;
    class LabelSequenceInteractionController;
    class NoteEditLayerInteractionController;
    class ClavierViewModel;
    class ClavierInteractionController;
}

namespace dspx {
    class SingingClip;
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
        Q_PROPERTY(sflow::LabelSequenceInteractionController *labelSequenceInteractionControllerOfTempo READ labelSequenceInteractionControllerOfTempo CONSTANT)
        Q_PROPERTY(sflow::LabelSequenceInteractionController *labelSequenceInteractionControllerOfLabel READ labelSequenceInteractionControllerOfLabel CONSTANT)
        Q_PROPERTY(sflow::NoteEditLayerInteractionController *noteEditLayerInteractionController READ noteEditLayerInteractionController CONSTANT)
        Q_PROPERTY(sflow::ClavierViewModel *clavierViewModel READ clavierViewModel CONSTANT)
        Q_PROPERTY(sflow::ClavierInteractionController *clavierInteractionController READ clavierInteractionController CONSTANT)
        Q_PROPERTY(PositionAlignmentManipulator *positionAlignmentManipulator READ positionAlignmentManipulator CONSTANT)
        Q_PROPERTY(AutoPageScrollingManipulator *autoPageScrollingManipulator READ autoPageScrollingManipulator CONSTANT)
        Q_PROPERTY(QQuickItem *pianoRollView READ pianoRollView CONSTANT)
        Q_PROPERTY(QAbstractItemModel *trackOverlaySelectorModel READ trackOverlaySelectorModel CONSTANT)
        Q_PROPERTY(QAbstractItemModel *editingClipSelectorModel READ editingClipSelectorModel CONSTANT)
        Q_PROPERTY(Tool tool READ tool WRITE setTool NOTIFY toolChanged)
        Q_PROPERTY(bool snapTemporarilyDisabled READ isSnapTemporarilyDisabled WRITE setSnapTemporarilyDisabled NOTIFY snapTemporarilyDisabledChanged)
        Q_PROPERTY(bool mouseTrackingDisabled READ isMouseTrackingDisabled WRITE setMouseTrackingDisabled NOTIFY mouseTrackingDisabledChanged)
        Q_PROPERTY(dspx::SingingClip *editingClip READ editingClip WRITE setEditingClip NOTIFY editingClipChanged)

    public:
        ~PianoRollPanelInterface() override;

        static PianoRollPanelInterface *of(const Core::ProjectWindowInterface *windowHandle);

        Core::ProjectWindowInterface *windowHandle() const;

        sflow::TimeViewModel *timeViewModel() const;
        sflow::TimeLayoutViewModel *timeLayoutViewModel() const;
        sflow::ScrollBehaviorViewModel *scrollBehaviorViewModel() const;
        sflow::TimelineInteractionController *timelineInteractionController() const;
        sflow::LabelSequenceInteractionController *labelSequenceInteractionControllerOfTempo() const;
        sflow::LabelSequenceInteractionController *labelSequenceInteractionControllerOfLabel() const;
        sflow::NoteEditLayerInteractionController *noteEditLayerInteractionController() const;
        sflow::ClavierViewModel *clavierViewModel() const;
        sflow::ClavierInteractionController *clavierInteractionController() const;

        PositionAlignmentManipulator *positionAlignmentManipulator() const;
        AutoPageScrollingManipulator *autoPageScrollingManipulator() const;

        QQuickItem *pianoRollView() const;
        QAbstractItemModel *trackOverlaySelectorModel() const;
        QAbstractItemModel *editingClipSelectorModel() const;

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

        dspx::SingingClip *editingClip() const;
        void setEditingClip(dspx::SingingClip *clip);

    Q_SIGNALS:
        void toolChanged();
        void snapTemporarilyDisabledChanged();
        void mouseTrackingDisabledChanged();
        void editingClipChanged();

    private:
        friend class Internal::PianoRollAddOn;
        explicit PianoRollPanelInterface(Internal::PianoRollAddOn *addOn, Core::ProjectWindowInterface *windowHandle);
        QScopedPointer<PianoRollPanelInterfacePrivate> d_ptr;
    };

}

#endif //DIFFSCOPE_VISUALEDITOR_PIANOROLLPANELINTERFACE_H
