// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_AUDIOBRIDGESERVER_CONTROLSERVER_H
#define DIFFSCOPE_AUDIOBRIDGESERVER_CONTROLSERVER_H

#include <memory>

#include <QHash>
#include <QObject>

#include <audiobridgeserver/internal/ControlCodec.h>

class QLocalServer;
class QLockFile;

namespace AudioBridge::Internal {

    class ControlServer : public QObject {
        Q_OBJECT
    public:
        ControlServer();
        ~ControlServer() override;

        void start();
        void stop();
        void send(quint64 connection, ControlOpcode opcode, quint64 id, quint16 flags, const QByteArray &body, bool close = false);
        void disconnectPeer(quint64 connection);

    Q_SIGNALS:
        void requestReceived(const AudioBridge::Internal::ControlRequest &request);
        void peerDisconnected(quint64 connection, int remainingLease);

    private:
        struct Peer;
        void acceptPeers();
        void readPeer(Peer *peer);
        void expirePeers();

        QLocalServer *m_server = nullptr;
        std::unique_ptr<QLockFile> m_endpointLock;
        QHash<quint64, Peer *> m_peers;
        quint64 m_nextConnection = 0;
    };

}

#endif
