#ifndef DIFFSCOPE_AUDIO_TRACKAUDIOCONTEXT_P_H
#define DIFFSCOPE_AUDIO_TRACKAUDIOCONTEXT_P_H

#include <audio/TrackAudioContext.h>

#include <QPointer>

namespace talcs {
    class DspxProjectContext;
    class DspxTrackContext;
}

namespace Audio {

    class TrackAudioContextPrivate {
        Q_DECLARE_PUBLIC(TrackAudioContext)
    public:
        TrackAudioContext *q_ptr{};
        Core::ProjectWindowInterface *windowHandle{};
        dspx::Track *track{};
        QPointer<talcs::DspxProjectContext> projectContext{};
        talcs::DspxTrackContext *trackContext{};

        static TrackAudioContext *create(Core::ProjectWindowInterface *windowHandle, dspx::Track *track, talcs::DspxProjectContext *projectContext, int index);
        static TrackAudioContextPrivate *of(TrackAudioContext *q);
    };

}

#endif // DIFFSCOPE_AUDIO_TRACKAUDIOCONTEXT_P_H
