#ifndef DIFFSCOPE_COREPLUGIN_EDITLOOPSCENARIO_P_H
#define DIFFSCOPE_COREPLUGIN_EDITLOOPSCENARIO_P_H

#include <coreplugin/EditLoopScenario.h>

class QQmlComponent;

namespace Core {

    class EditLoopScenarioPrivate {
        Q_DECLARE_PUBLIC(EditLoopScenario)
    public:
        EditLoopScenario *q_ptr;
        QQuickWindow *window = nullptr;
        ProjectTimeline *projectTimeline = nullptr;
        DspxDocument *document = nullptr;
        bool shouldDialogPopupAtCursor = false;

        QObject *createAndPositionDialog(QQmlComponent *component, int startPosition, int endPosition, bool loopEnabled) const;
        bool execDialog(QObject *dialog) const;
    };

}

#endif //DIFFSCOPE_COREPLUGIN_EDITLOOPSCENARIO_P_H
