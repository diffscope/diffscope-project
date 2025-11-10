#ifndef DIFFSCOPE_VISUALEDITOR_ARRANGEMENTPANELINTERFACE_H
#define DIFFSCOPE_VISUALEDITOR_ARRANGEMENTPANELINTERFACE_H

#include <QObject>

#include <visualeditor/visualeditorglobal.h>

namespace Core {
    class ProjectWindowInterface;
}

namespace VisualEditor {

    class ArrangementPanelInterfacePrivate;

    class VISUAL_EDITOR_EXPORT ArrangementPanelInterface : public QObject {
        Q_OBJECT
        Q_DECLARE_PRIVATE(ArrangementPanelInterface)

    public:
        explicit ArrangementPanelInterface(Core::ProjectWindowInterface *windowHandle);
        ~ArrangementPanelInterface() override;

        static ArrangementPanelInterface *of(const Core::ProjectWindowInterface *windowHandle);

    private:
        QScopedPointer<ArrangementPanelInterfacePrivate> d_ptr;
    };

}

#endif //DIFFSCOPE_VISUALEDITOR_ARRANGEMENTPANELINTERFACE_H