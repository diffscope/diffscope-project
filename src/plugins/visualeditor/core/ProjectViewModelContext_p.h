#ifndef DIFFSCOPE_COREPLUGIN_PROJECTVIEWMODELCONTEXT_P_H
#define DIFFSCOPE_COREPLUGIN_PROJECTVIEWMODELCONTEXT_P_H

#include <memory>

#include <visualeditor/ProjectViewModelContext.h>

namespace VisualEditor {

    class PlaybackViewModelContextData;
    class TempoViewModelContextData;
    class LabelViewModelContextData;
    class TrackViewModelContextData;
    class MasterTrackViewModelContextData;

    class ProjectViewModelContextAttachedType : public QObject {
        Q_OBJECT
        QML_ANONYMOUS
        Q_PROPERTY(ProjectViewModelContext *context READ context CONSTANT)

    public:
        explicit ProjectViewModelContextAttachedType(QObject *parent = nullptr);
        ~ProjectViewModelContextAttachedType() override;

        ProjectViewModelContext *context() const;
    };

    class ProjectViewModelContextPrivate {
        Q_DECLARE_PUBLIC(ProjectViewModelContext)
    public:
        ProjectViewModelContext *q_ptr;

        Core::ProjectWindowInterface *windowHandle;

        std::unique_ptr<PlaybackViewModelContextData> playbackData;
        std::unique_ptr<TempoViewModelContextData> tempoData;
        std::unique_ptr<LabelViewModelContextData> labelData;
        std::unique_ptr<TrackViewModelContextData> trackData;
        std::unique_ptr<MasterTrackViewModelContextData> masterTrackData;
    };
}

#endif //DIFFSCOPE_COREPLUGIN_PROJECTVIEWMODELCONTEXT_P_H
