#ifndef DIFFSCOPE_COREPLUGIN_OPENSAVEPROJECTFILESCENARIO_P_H
#define DIFFSCOPE_COREPLUGIN_OPENSAVEPROJECTFILESCENARIO_P_H

#include <coreplugin/OpenSaveProjectFileScenario.h>

namespace Core {

    class OpenSaveProjectFileScenarioPrivate {
        Q_DECLARE_PUBLIC(OpenSaveProjectFileScenario)
    public:
        OpenSaveProjectFileScenario *q_ptr;
        QWindow *window{};
    };

}

#endif //DIFFSCOPE_COREPLUGIN_OPENSAVEPROJECTFILESCENARIO_P_H
