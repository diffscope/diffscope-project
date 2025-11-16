#ifndef DIFFSCOPE_VISUALEDITOR_ARRANGEMENTPANELINTERFACE_H
#define DIFFSCOPE_VISUALEDITOR_ARRANGEMENTPANELINTERFACE_H

#include <QObject>
#include <qqmlintegration.h>

#include <visualeditor/visualeditorglobal.h>

class QQuickItem;

namespace sflow {
    class TimeViewModel;
    class TimeLayoutViewModel;
    class TimelineInteractionController;
    class ScrollBehaviorViewModel;
}

namespace Core {
    class ProjectWindowInterface;
}

namespace VisualEditor {

    namespace Internal {
        class ArrangementAddOn;
    }

    class PositionAlignmentManipulator;
    class AutoPageScrollingManipulator;

    class ArrangementPanelInterfacePrivate;

    class VISUAL_EDITOR_EXPORT ArrangementPanelInterface : public QObject {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_DECLARE_PRIVATE(ArrangementPanelInterface)
        Q_PROPERTY(Core::ProjectWindowInterface *windowHandle READ windowHandle CONSTANT)
        Q_PROPERTY(sflow::TimeViewModel *timeViewModel READ timeViewModel CONSTANT)
        Q_PROPERTY(sflow::TimeLayoutViewModel *timeLayoutViewModel READ timeLayoutViewModel CONSTANT)
        Q_PROPERTY(sflow::ScrollBehaviorViewModel *scrollBehaviorViewModel READ scrollBehaviorViewModel CONSTANT)
        Q_PROPERTY(sflow::TimelineInteractionController *timelineInteractionController READ timelineInteractionController CONSTANT)
        Q_PROPERTY(PositionAlignmentManipulator *positionAlignmentManipulator READ positionAlignmentManipulator CONSTANT)
        Q_PROPERTY(AutoPageScrollingManipulator *autoPageScrollingManipulator READ autoPageScrollingManipulator CONSTANT)
        Q_PROPERTY(QQuickItem *arrangementView READ arrangementView CONSTANT)
        Q_PROPERTY(Tool tool READ tool WRITE setTool NOTIFY toolChanged)
        Q_PROPERTY(bool snapTemporarilyDisabled READ isSnapTemporarilyDisabled WRITE setSnapTemporarilyDisabled NOTIFY snapTemporarilyDisabledChanged)

    public:
        ~ArrangementPanelInterface() override;

        static ArrangementPanelInterface *of(const Core::ProjectWindowInterface *windowHandle);

        Core::ProjectWindowInterface *windowHandle() const;

        sflow::TimeViewModel *timeViewModel() const;
        sflow::TimeLayoutViewModel *timeLayoutViewModel() const;
        sflow::ScrollBehaviorViewModel *scrollBehaviorViewModel() const;
        sflow::TimelineInteractionController *timelineInteractionController() const;

        PositionAlignmentManipulator *positionAlignmentManipulator() const;
        AutoPageScrollingManipulator *autoPageScrollingManipulator() const;

        QQuickItem *arrangementView() const;

        enum Tool {
            PointerTool,
            PencilTool,
            HandTool,
        };
        Q_ENUM(Tool)
        Tool tool() const;
        void setTool(Tool tool);

        bool isSnapTemporarilyDisabled() const;
        void setSnapTemporarilyDisabled(bool disabled);

    Q_SIGNALS:
        void toolChanged();
        void snapTemporarilyDisabledChanged();

    private:
        friend class Internal::ArrangementAddOn;
        QScopedPointer<ArrangementPanelInterfacePrivate> d_ptr;
        explicit ArrangementPanelInterface(Core::ProjectWindowInterface *windowHandle);
    };

}

#endif //DIFFSCOPE_VISUALEDITOR_ARRANGEMENTPANELINTERFACE_H