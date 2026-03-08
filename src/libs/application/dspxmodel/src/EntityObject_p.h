#ifndef DIFFSCOPE_DSPX_MODEL_ENTITYOBJECT_P_H
#define DIFFSCOPE_DSPX_MODEL_ENTITYOBJECT_P_H

#include <QPointer>

#include <dspxmodel/EntityObject.h>

namespace dspx {

    class EntityObjectPrivate {
        Q_DECLARE_PUBLIC(EntityObject)
    public:
        EntityObject *q_ptr;
        QPointer<Model> model{};
        Handle handle;
    };

}

#endif //DIFFSCOPE_DSPX_MODEL_ENTITYOBJECT_P_H
