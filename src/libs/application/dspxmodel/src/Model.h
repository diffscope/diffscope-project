#ifndef DIFFSCOPE_DSPX_MODEL_MODEL_H
#define DIFFSCOPE_DSPX_MODEL_MODEL_H

#include <qqmlintegration.h>

#include <dspxmodel/EntityObject.h>

namespace dspx {

    class ModelStrategy;

    class ModelPrivate;

    class Global;
    class Master;
    class Timeline;
    class TrackList;
    class Workspace;

    class DSPX_MODEL_EXPORT Model : public EntityObject {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_DECLARE_PRIVATE(Model)

        Q_PROPERTY(Global *global READ global CONSTANT)
        Q_PROPERTY(Master *master READ master CONSTANT)
        Q_PROPERTY(Timeline *timeline READ timeline CONSTANT)
        Q_PROPERTY(TrackList *trackList READ trackList CONSTANT)
        Q_PROPERTY(Workspace *workspace READ workspace CONSTANT)

    public:
        explicit Model(ModelStrategy *strategy, QObject *parent = nullptr);
        ~Model() override;

        ModelStrategy *strategy() const;

        Global *global() const;
        Master *master() const;
        Timeline *timeline() const;
        TrackList *trackList() const;
        Workspace *workspace() const;

    protected:
        void handleSetEntityProperty(int property, const QVariant &value) override;

    private:
        QScopedPointer<ModelPrivate> d_ptr;

    };

}

#endif //DIFFSCOPE_DSPX_MODEL_MODEL_H
