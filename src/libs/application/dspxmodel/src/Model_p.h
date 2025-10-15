#ifndef DIFFSCOPE_DSPX_MODEL_MODEL_P_H
#define DIFFSCOPE_DSPX_MODEL_MODEL_P_H

#include <QHash>

#include <dspxmodel/Model.h>

namespace dspx {

    class LabelSequence;
    class TempoSequence;
    class TimeSignatureSequence;

    class ModelPrivate {
        Q_DECLARE_PUBLIC(Model)
    public:
        Model *q_ptr;

        ModelStrategy *strategy;
        Global *global;
        Master *master;
        Timeline *timeline;

        LabelSequence *labels;
        TempoSequence *tempos;
        TimeSignatureSequence *timeSignatures;
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

        static ModelPrivate *get(Model *model) {
            return model->d_func();
        }

        void handleEntityDestroyed(Handle handle);
        void init();
        void handleNotifications();

        EntityObject *mapToObject(Handle handle) const;
        Handle mapToHandle(EntityObject *object) const;

        template<class T>
        T *createObject(Handle handle) {
            Q_Q(Model);
            return new T(handle, q);
        }
    };

}

#endif //DIFFSCOPE_DSPX_MODEL_MODEL_P_H
