#ifndef DIFFSCOPE_AUDIO_GLOBALAUDIOCONTEXT_P_H
#define DIFFSCOPE_AUDIO_GLOBALAUDIOCONTEXT_P_H

#include <audio/GlobalAudioContext.h>

#include <memory>

namespace Audio {

    class GlobalAudioContextPrivate {
        Q_DECLARE_PUBLIC(GlobalAudioContext)
    public:
        GlobalAudioContext *q_ptr{};

        std::unique_ptr<talcs::FormatManager> formatManager;

        static GlobalAudioContext *create(QObject *parent = nullptr);
    };

}

#endif // DIFFSCOPE_AUDIO_GLOBALAUDIOCONTEXT_P_H
