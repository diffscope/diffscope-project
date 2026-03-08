#ifndef DIFFSCOPE_COREPLUGIN_INSERTITEMSCENARIO_P_H
#define DIFFSCOPE_COREPLUGIN_INSERTITEMSCENARIO_P_H

#include <coreplugin/InsertItemScenario.h>

class QQmlComponent;

namespace Core {

    class InsertItemScenarioPrivate {
        Q_DECLARE_PUBLIC(InsertItemScenario)
    public:
        InsertItemScenario *q_ptr;
        ProjectTimeline *projectTimeline = nullptr;
    };

}

#endif //DIFFSCOPE_COREPLUGIN_INSERTITEMSCENARIO_P_H