#ifndef DIFFSCOPE_COREPLUGIN_EDITTEMPOTIMESIGNATURESCENARIO_P_H
#define DIFFSCOPE_COREPLUGIN_EDITTEMPOTIMESIGNATURESCENARIO_P_H

#include <coreplugin/EditTempoTimeSignatureScenario.h>

class QQmlComponent;

namespace Core {

    class EditTempoTimeSignatureScenarioPrivate {
        Q_DECLARE_PUBLIC(EditTempoTimeSignatureScenario)
    public:
        EditTempoTimeSignatureScenario *q_ptr;
        ProjectTimeline *projectTimeline = nullptr;
    };

}

#endif //DIFFSCOPE_COREPLUGIN_EDITTEMPOTIMESIGNATURESCENARIO_P_H
