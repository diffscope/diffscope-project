// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_AUDIOBRIDGESERVER_BRIDGESERVER_H
#define DIFFSCOPE_AUDIOBRIDGESERVER_BRIDGESERVER_H

#include <memory>

#include <QHash>
#include <QObject>
#include <QThread>

#include <audiobridgeserver/internal/ControlCodec.h>

namespace AudioBridge::Internal {

    class AudioIpc;
    class BridgeSession;
    class ControlServer;

    class BridgeServer : public QObject {
    public:
        explicit BridgeServer(QObject *parent = nullptr);
        ~BridgeServer() override;

        void start();
        void stop();
        void reply(const ControlRequest &request, const QByteArray &body = {});
        void error(const ControlRequest &request, ControlErrorCode code, const QString &detail = {}, bool close = false);
        void status(quint64 connection, const QByteArray &body);

    private:
        void receive(const ControlRequest &request);
        void attach(const ControlRequest &request);
        void disconnected(quint64 connection, int remainingLease);
        void closeSession(BridgeSession *session, bool preserveRecoveryLog);
        void send(quint64 connection, ControlOpcode opcode, quint64 id, quint16 flags, const QByteArray &body,
                  bool close = false);

        QThread m_controlThread;
        std::unique_ptr<AudioIpc> m_audio;
        ControlServer *m_control{};
        QHash<QByteArray, BridgeSession *> m_sessions;
        QHash<quint64, BridgeSession *> m_connections;
        bool m_stopping{};
    };

}

#endif
