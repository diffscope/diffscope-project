// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_AUDIO_GLOBALAUDIOCONTEXT_P_H
#define DIFFSCOPE_AUDIO_GLOBALAUDIOCONTEXT_P_H

#include <audio/GlobalAudioContext.h>

#include <QMutex>

#include <memory>

namespace Audio {

    class GlobalAudioContextPrivate {
        Q_DECLARE_PUBLIC(GlobalAudioContext)
    public:
        GlobalAudioContext *q_ptr{};

        std::unique_ptr<talcs::FormatManager> formatManager;
        mutable QMutex propertiesMutex;
        bool metronomeEnabled{};
        double metronomeGain{1.0};
        double metronomePan{};
        double deviceGain{1.0};
        double devicePan{};

        static GlobalAudioContext *create(QObject *parent = nullptr);
    };

}

#endif // DIFFSCOPE_AUDIO_GLOBALAUDIOCONTEXT_P_H
