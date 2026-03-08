#ifndef DIFFSCOPE_COREPLUGIN_EDITKEYSIGNATURESCENARIO_P_H
#define DIFFSCOPE_COREPLUGIN_EDITKEYSIGNATURESCENARIO_P_H

#include <coreplugin/EditKeySignatureScenario.h>

class QQmlComponent;

namespace Core {

    class EditKeySignatureScenarioPrivate {
        Q_DECLARE_PUBLIC(EditKeySignatureScenario)
    public:
        EditKeySignatureScenario *q_ptr;
        ProjectTimeline *projectTimeline = nullptr;
    };

}

#endif //DIFFSCOPE_COREPLUGIN_EDITKEYSIGNATURESCENARIO_P_H
