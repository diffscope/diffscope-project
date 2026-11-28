// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_AUDIOBRIDGESERVER_AUDIOIPC_H
#define DIFFSCOPE_AUDIOBRIDGESERVER_AUDIOIPC_H

#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <thread>

#include <QByteArray>
#include <QFuture>
#include <QPromise>

#include <audiobridgeserver/internal/ControlCodec.h>

namespace Audio {
    class ProjectAudioRenderSession;
}

namespace AudioBridge::Internal {

    class AudioPort {
    public:
        enum class Mode { Idle, Host, Preview, Offline };

        AudioPort(quint32 maxFrames, int channels, std::function<void()> notifyCompletion);
        ~AudioPort();

        const QByteArray audioName;
        const QByteArray wakeName;
        const quint32 maxFrames;
        const int channels;

        void configure(Audio::ProjectAudioRenderSession *render, Mode mode, quint32 generation, qint64 position);
        void suspend();
        std::optional<QFuture<void>> suspendAsynchronously();
        qint64 position() const;
        AudioReplyStatus check(const AudioRequestV1 &request) const;
        AudioReplyStatus enqueue(const AudioRequestV1 &request, float *samples);
        std::optional<AudioReplyStatus> takeResult();
        void complete(const AudioRequestV1 &request, const std::function<bool()> &send);

    private:
        struct Job {
            qint64 position;
            quint32 frames;
            float *samples;
        };

        AudioReplyStatus checkLocked(const AudioRequestV1 &request) const;
        void invalidateLocked();
        void renderLoop();

        mutable std::mutex m_mutex;
        std::condition_variable m_changed;
        Audio::ProjectAudioRenderSession *m_render{};
        Mode m_mode{Mode::Idle};
        quint32 m_generation{};
        qint64 m_position{};
        bool m_rendering{};
        std::optional<Job> m_job;
        std::optional<AudioReplyStatus> m_result;
        std::optional<QPromise<void>> m_suspension;
        const std::function<void()> m_notifyCompletion;
        bool m_stopping{};
        std::thread m_thread;
    };

    class AudioIpc {
    public:
        AudioIpc();
        ~AudioIpc();

        std::shared_ptr<AudioPort> open(quint32 maxFrames, int channels);
        void close(const std::shared_ptr<AudioPort> &port);

    private:
        struct Dispatcher;
        std::unique_ptr<Dispatcher> m_dispatcher;
    };

}

#endif
