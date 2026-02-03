#ifndef DIFFSCOPE_COREPLUGIN_EDITLOOPSCENARIO_P_H
#define DIFFSCOPE_COREPLUGIN_EDITLOOPSCENARIO_P_H

#include <coreplugin/EditLoopScenario.h>

class QQmlComponent;

namespace Core {

    class EditLoopScenarioPrivate {
        Q_DECLARE_PUBLIC(EditLoopScenario)
    public:
        EditLoopScenario *q_ptr;
        ProjectTimeline *projectTimeline = nullptr;
    };

}

#endif //DIFFSCOPE_COREPLUGIN_EDITLOOPSCENARIO_P_H
