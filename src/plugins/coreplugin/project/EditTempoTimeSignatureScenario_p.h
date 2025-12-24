#ifndef DIFFSCOPE_COREPLUGIN_EDITTEMPOTIMESIGNATURESCENARIO_P_H
#define DIFFSCOPE_COREPLUGIN_EDITTEMPOTIMESIGNATURESCENARIO_P_H

#include <coreplugin/EditTempoTimeSignatureScenario.h>

class QQmlComponent;

namespace Core {

    class EditTempoTimeSignatureScenarioPrivate {
        Q_DECLARE_PUBLIC(EditTempoTimeSignatureScenario)
    public:
        EditTempoTimeSignatureScenario *q_ptr;
        QQuickWindow *window = nullptr;
        ProjectTimeline *projectTimeline = nullptr;
        DspxDocument *document = nullptr;
        bool shouldDialogPopupAtCursor = false;

        QObject *createAndPositionDialog(QQmlComponent *component, int position, bool doInsertNew) const;
        bool execDialog(QObject *dialog) const;
    };

}

#endif //DIFFSCOPE_COREPLUGIN_EDITTEMPOTIMESIGNATURESCENARIO_P_H
