#ifndef DIFFSCOPE_VISUALEDITOR_ARRANGEMENTPANELINTERFACE_P_H
#define DIFFSCOPE_VISUALEDITOR_ARRANGEMENTPANELINTERFACE_P_H

#include <visualeditor/ArrangementPanelInterface.h>

namespace VisualEditor {

    class ArrangementPanelInterfacePrivate {
        Q_DECLARE_PUBLIC(ArrangementPanelInterface)
    public:
        ArrangementPanelInterface *q_ptr;

        Core::ProjectWindowInterface *windowHandle;
    };

}

#endif //DIFFSCOPE_VISUALEDITOR_ARRANGEMENTPANELINTERFACE_P_H