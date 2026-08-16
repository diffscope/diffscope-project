// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_AUDIO_PROJECTAUDIOADDON_H
#define DIFFSCOPE_AUDIO_PROJECTAUDIOADDON_H

#include <CoreApi/windowinterface.h>

#include <QHash>
#include <QMutex>
#include <QSet>
#include <QVector>

#include <TalcsCore/MetronomeAudioSource.h>

#include <memory>

namespace Core {
    class ProjectWindowInterface;
}

namespace dspx {
    class AudioClip;
    class Clip;
    class Tempo;
    class TimeSignature;
    class Track;
}

namespace talcs {
    class AbstractAudioFormatIO;
    class MetronomeAudioSource;
}

namespace SVS {
    class MusicTimeline;
}

namespace Audio {
    class AudioClipAudioContext;
    class ProjectAudioContext;
    class TrackAudioContext;
}

namespace Audio::Internal {

    class ProjectAudioAddOn : public Core::WindowInterfaceAddOn, private talcs::MetronomeAudioSourceDetector {
        Q_OBJECT
    public:
        explicit ProjectAudioAddOn(QObject *parent = nullptr);
        ~ProjectAudioAddOn() override;

        void initialize() override;
        void extensionsInitialized() override;
        bool delayedInitialize() override;

        static ProjectAudioAddOn *of(Core::ProjectWindowInterface *windowHandle);

        void addAudioClipCache(dspx::AudioClip *clip, talcs::AbstractAudioFormatIO *io);
        talcs::AbstractAudioFormatIO *takeAudioClipCache(dspx::AudioClip *clip);

    private Q_SLOTS:
        void addTrack(int index, dspx::Track *track);
        void removeTrack(int index, dspx::Track *track);
        void rotateTrack(int leftIndex, int middleIndex, int rightIndex);

    private:
        void syncMasterControl();
        void syncTrackControl(dspx::Track *track, TrackAudioContext *context);
        void syncTrackClips(dspx::Track *track, TrackAudioContext *context);
        void addClip(dspx::Clip *clip);
        void removeClip(dspx::Clip *clip);
        void syncAudioClip(dspx::AudioClip *clip, AudioClipAudioContext *context);
        void loadAudioClip(dspx::AudioClip *clip, AudioClipAudioContext *context);
        void reloadAudioClip(dspx::AudioClip *clip, AudioClipAudioContext *context);
        void notifyAudioClipStatus(dspx::AudioClip *clip, AudioClipAudioContext *context);

        void handleTempoInsertedOrUpdated(dspx::Tempo *tempo);
        void handleTempoRemoved(dspx::Tempo *tempo);
        void handleTimeSignatureInsertedOrUpdated(dspx::TimeSignature *timeSignature);
        void handleTimeSignatureRemoved(dspx::TimeSignature *timeSignature);

        void detectInterval(qint64 intervalLength) override;
        talcs::MetronomeAudioSourceDetectorMessage nextMessage() override;

        ProjectAudioContext *m_context{};
        QHash<dspx::AudioClip *, talcs::AbstractAudioFormatIO *> m_audioClipCache;

        std::unique_ptr<SVS::MusicTimeline> m_musicTimeline;
        QMutex m_musicTimelineMutex;
        QHash<int, QSet<dspx::Tempo *>> m_tempoMap;
        QHash<dspx::Tempo *, int> m_tempoPosMap;
        QHash<int, QSet<dspx::TimeSignature *>> m_timeSignatureMap;
        QHash<dspx::TimeSignature *, int> m_timeSignatureMeasureMap;

        talcs::MetronomeAudioSource *m_metronomeAudioSource{};
        QMutex m_transportPositionMutex;
        qint64 m_transportPosition{};
        QVector<talcs::MetronomeAudioSourceDetectorMessage> m_metronomeMessages;
        qsizetype m_nextMetronomeMessageIndex{};
    };

}

#endif // DIFFSCOPE_AUDIO_PROJECTAUDIOADDON_H
