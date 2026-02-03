#ifndef DIFFSCOPE_COREPLUGIN_EDITMETADATASCENARIO_P_H
#define DIFFSCOPE_COREPLUGIN_EDITMETADATASCENARIO_P_H

#include <coreplugin/EditMetadataScenario.h>

class QQmlComponent;

namespace Core {

    class EditMetadataScenarioPrivate {
        Q_DECLARE_PUBLIC(EditMetadataScenario)
    public:
        EditMetadataScenario *q_ptr;
    };

}

#endif //DIFFSCOPE_COREPLUGIN_EDITMETADATASCENARIO_P_H
