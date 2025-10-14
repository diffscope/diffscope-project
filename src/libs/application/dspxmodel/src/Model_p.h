#ifndef DIFFSCOPE_DSPX_MODEL_MODEL_P_H
#define DIFFSCOPE_DSPX_MODEL_MODEL_P_H

#include <QHash>

#include <dspxmodel/Model.h>

namespace dspx {

    class LabelSequence;

    class ModelPrivate {
        Q_DECLARE_PUBLIC(Model)
    public:
        Model *q_ptr;

        ModelStrategy *strategy;
        Global *global;
        Master *master;
        Timeline *timeline;
        TrackList *trackList;
        Workspace *workspace;

        QHash<Handle, EntityObject *> objectMap;
        QHash<EntityObject *, Handle> handleMap;

        QString name;
        QString author;
        int centShift;
        QString editorId;
        QString editorName;

        double gain;
        double pan;
        bool mute;

        LabelSequence *labels;

        static ModelPrivate *get(Model *model) {
            return model->d_func();
        }

        void handleEntityDestroyed(Handle handle);
        void init();

        EntityObject *mapToObject(Handle handle) const;
        Handle mapToHandle(EntityObject *object) const;

        Label *createLabel(Handle handle);
    };

}

#endif //DIFFSCOPE_DSPX_MODEL_MODEL_P_H
