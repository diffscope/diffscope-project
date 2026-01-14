#ifndef DIFFSCOPE_COREPLUGIN_INSERTITEMSCENARIO_P_H
#define DIFFSCOPE_COREPLUGIN_INSERTITEMSCENARIO_P_H

#include <coreplugin/InsertItemScenario.h>

class QQmlComponent;

namespace Core {

    class InsertItemScenarioPrivate {
        Q_DECLARE_PUBLIC(InsertItemScenario)
    public:
        InsertItemScenario *q_ptr;
        QQuickWindow *window = nullptr;
        ProjectTimeline *projectTimeline = nullptr;
        DspxDocument *document = nullptr;
        bool shouldDialogPopupAtCursor = false;

        QObject *createAndPositionDialog(QQmlComponent *component, const QVariantMap &initialProperties) const;
        bool execDialog(QObject *dialog) const;
    };

}

#endif //DIFFSCOPE_COREPLUGIN_INSERTITEMSCENARIO_P_H