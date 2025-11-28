#ifndef DIFFSCOPE_DSPX_MODEL_MODEL_H
#define DIFFSCOPE_DSPX_MODEL_MODEL_H

#include <qqmlintegration.h>

#include <dspxmodel/EntityObject.h>

namespace QDspx {
    struct Model;
}

namespace dspx {

    class ModelStrategy;

    class ModelPrivate;

    class Global;
    class Master;
    class Timeline;
    class TrackList;
    class Workspace;
    class Label;
    class Note;
    class Phoneme;
    class Tempo;
    class TimeSignature;
    class Track;
    class WorkspaceInfo;
    class AudioClip;
    class SingingClip;
    class AnchorNode;
    class ParamCurveAnchor;
    class ParamCurveFree;
    class Param;
    class Source;

    class DSPX_MODEL_EXPORT Model : public EntityObject {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_DECLARE_PRIVATE(Model)

        Q_PROPERTY(Global *global READ global CONSTANT)
        Q_PROPERTY(Master *master READ master CONSTANT)
        Q_PROPERTY(Timeline *timeline READ timeline CONSTANT)
        Q_PROPERTY(TrackList *tracks READ tracks CONSTANT)
        Q_PROPERTY(Workspace *workspace READ workspace CONSTANT)

    public:
        explicit Model(ModelStrategy *strategy, QObject *parent = nullptr);
        ~Model() override;

        ModelStrategy *strategy() const;

        Global *global() const;
        Master *master() const;
        Timeline *timeline() const;
        TrackList *tracks() const;
        Workspace *workspace() const;

        QDspx::Model toQDspx() const;
        void fromQDspx(const QDspx::Model &model);

        Q_INVOKABLE Label *createLabel();
        Q_INVOKABLE Note *createNote();
        Q_INVOKABLE Phoneme *createPhoneme();
        Q_INVOKABLE Tempo *createTempo();
        Q_INVOKABLE TimeSignature *createTimeSignature();
        Q_INVOKABLE Track *createTrack();
        Q_INVOKABLE WorkspaceInfo *createWorkspaceInfo();
        Q_INVOKABLE AudioClip *createAudioClip();
        Q_INVOKABLE SingingClip *createSingingClip();
        Q_INVOKABLE AnchorNode *createAnchorNode();
        Q_INVOKABLE ParamCurveAnchor *createParamCurveAnchor();
        Q_INVOKABLE ParamCurveFree *createParamCurveFree();
        Q_INVOKABLE Param *createParam();
        Q_INVOKABLE Source *createSource();

    protected:
        void handleSetEntityProperty(int property, const QVariant &value) override;

    private:
        QScopedPointer<ModelPrivate> d_ptr;
    };

}

#endif //DIFFSCOPE_DSPX_MODEL_MODEL_H
