#ifndef DIFFSCOPE_COREPLUGIN_DOCUMENTEDITSCENARIO_P_H
#define DIFFSCOPE_COREPLUGIN_DOCUMENTEDITSCENARIO_P_H

#include <coreplugin/DocumentEditScenario.h>

namespace Core {

    class DocumentEditScenarioPrivate {
        Q_DECLARE_PUBLIC(DocumentEditScenario)
    public:
        DocumentEditScenario *q_ptr;
        QQuickWindow *window = nullptr;
        DspxDocument *document = nullptr;
        bool shouldDialogPopupAtCursor = false;

        static bool execDialog(QObject *dialog);
    };

}

#endif //DIFFSCOPE_COREPLUGIN_DOCUMENTEDITSCENARIO_P_H
