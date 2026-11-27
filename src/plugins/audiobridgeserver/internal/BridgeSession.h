// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_AUDIOBRIDGESERVER_BRIDGESESSION_H
#define DIFFSCOPE_AUDIOBRIDGESERVER_BRIDGESESSION_H

#include <memory>
#include <optional>

#include <QMetaObject>
#include <QObject>
#include <QPointer>
#include <QTimer>

#include <audio/ProjectAudioContext.h>

#include <audiobridgeserver/internal/ControlCodec.h>

class QFutureWatcherBase;

namespace Core {
    class ProjectWindowInterface;
}

namespace DspxProject {
    struct ProjectFileError;
}

namespace AudioBridge::Internal {

    class AudioIpc;
    class AudioPort;
    class BridgeServer;

    class BridgeSession : public QObject {
    public:
        BridgeSession(BridgeServer *server, AudioIpc *audio, const QByteArray &instanceId);
        ~BridgeSession() override;

        const QByteArray instanceId;
        quint64 connection() const;
        bool canAttach() const;
        bool initialize(BridgeStateKind kind);
        void attach(quint64 connection, quint32 stateLimit, bool audioCompatible, bool visible, const QByteArray &packet);
        void disconnect(int remainingLease);
        void close(bool preserveRecoveryLog, bool waitForAudio = false);
        void dispatch(const ControlRequest &request);
        QByteArray statusBody() const;

        QTimer lease;

    private:
        struct StateResult {
            ControlErrorCode code = ControlErrorCode::None;
            QString detail;
        };

        void publishStatus();
        void setVisible(bool visible, bool activate = false);
        void watchFilePath();
        void configureAudio(const ControlRequest &request);
        void transport(const ControlRequest &request);
        ControlErrorCode localTransport(LocalTransportCommand command, qint64 position = 0);
        void beginOffline(const ControlRequest &request);
        void endOffline(const ControlRequest &request);
        void cancelOffline();
        void resumeRealtime();
        void releaseRender();
        void releaseAudio();
        void releaseAudioWhenIdle();
        void publishGeneration();
        void updateTimeline();
        qint64 timelinePosition() const;
        void saveState(const ControlRequest &request);
        void projectAction(const ControlRequest &request);
        void replyWithState(const ControlRequest &request, const QByteArray &data, const DspxProject::ProjectFileError &error);
        StateResult restoreState(const QByteArray &packet);
        void finishClose();
        void documentReloading();
        void documentReloaded();
        void audioSessionChanged();

        BridgeServer *m_server;
        AudioIpc *m_audioIpc;
        QPointer<Core::ProjectWindowInterface> m_window;
        QPointer<Audio::ProjectAudioContext> m_audio;
        std::shared_ptr<AudioPort> m_port;
        std::unique_ptr<Audio::ProjectAudioRenderSession> m_render;
        Audio::ProjectAudioRenderConfig m_config;
        QTimer m_timelineTimer;
        QMetaObject::Connection m_filePathConnection;
        std::unique_ptr<QObject> m_projectDialog;
        QFutureWatcherBase *m_offlineWatcher{};
        QFutureWatcherBase *m_audioDrainWatcher{};
        std::optional<ControlRequest> m_offlineRequest;
        quint64 m_connection{};
        quint32 m_stateLimit{};
        quint32 m_generation{};
        quint32 m_nextGeneration{};
        qint64 m_hostPosition{};
        BridgeStateKind m_kind{BridgeStateKind::HostState};
        HostTransportState m_host{HostTransportState::Unknown};
        AudioPortState m_audioState{AudioPortState::Unconfigured};
        ProjectLoadState m_loadState{ProjectLoadState::Ready};
        StateResult m_lastError;
        bool m_audioCompatible{};
        bool m_attaching{};
        bool m_changingRender{};
        bool m_persistence{};
        bool m_projectActionPending{};
        bool m_reloading{};
        bool m_closing{};
        bool m_preserveRecoveryLog{};
    };

}

#endif
