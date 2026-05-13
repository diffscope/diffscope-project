#ifndef DIFFSCOPE_AUDIO_PROJECTAUDIOCONTEXT_P_H
#define DIFFSCOPE_AUDIO_PROJECTAUDIOCONTEXT_P_H

#include <audio/ProjectAudioContext.h>

#include <memory>

namespace talcs {
    class DspxProjectContext;
}

namespace Audio {

    class ProjectAudioContextPrivate {
        Q_DECLARE_PUBLIC(ProjectAudioContext)
    public:
        ProjectAudioContext *q_ptr{};
        Core::ProjectWindowInterface *windowHandle{};
        ProjectAudioContext::PlaybackStatus status{ProjectAudioContext::Stopped};
        std::unique_ptr<talcs::DspxProjectContext> projectContext;

        static ProjectAudioContext *create(Core::ProjectWindowInterface *windowHandle);
        static ProjectAudioContextPrivate *of(ProjectAudioContext *q);
    };

}

#endif // DIFFSCOPE_AUDIO_PROJECTAUDIOCONTEXT_P_H
