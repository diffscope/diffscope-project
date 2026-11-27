// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "ControlServer.h"

#include <limits>

#ifdef Q_OS_UNIX
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

#include <QDeadlineTimer>
#include <QDir>
#include <QFile>
#include <QLocalServer>
#include <QLocalSocket>
#include <QLockFile>
#include <QLoggingCategory>
#include <QTimer>

namespace AudioBridge::Internal {

    Q_STATIC_LOGGING_CATEGORY(lcBridgeControl, "diffscope.audiobridgeserver.control")

    namespace {

        constexpr int leaseCheckIntervalMs = 250;

        QString endpointName() {
#ifdef Q_OS_WIN
            return QString::fromLatin1(BridgeControlEndpointName);
#else
#ifdef Q_OS_MACOS
            QByteArray runtimeDirectory;
            const auto size = confstr(_CS_DARWIN_USER_TEMP_DIR, nullptr, 0);
            if (size == 0)
                return {};
            runtimeDirectory.resize(size);
            if (confstr(_CS_DARWIN_USER_TEMP_DIR, runtimeDirectory.data(), size) != size)
                return {};
            runtimeDirectory.chop(1);
#else
            const auto runtimeDirectory = qgetenv("XDG_RUNTIME_DIR");
#endif
            struct stat info{};
            const auto path = QFile::decodeName(runtimeDirectory);
            if (!QDir::isAbsolutePath(path) || stat(runtimeDirectory.constData(), &info) != 0 ||
                !S_ISDIR(info.st_mode) || info.st_uid != geteuid() || (info.st_mode & (S_IRWXG | S_IRWXO)) != 0)
                return {};
            return QDir(path).filePath(QString::fromLatin1(BridgeControlEndpointName));
#endif
        }

        bool sameUser(const QLocalSocket *socket) {
#if defined(Q_OS_LINUX)
            struct ucred credentials{};
            socklen_t size = sizeof(credentials);
            return getsockopt(static_cast<int>(socket->socketDescriptor()), SOL_SOCKET, SO_PEERCRED, &credentials, &size) == 0 &&
                   size == sizeof(credentials) && credentials.uid == geteuid();
#elif defined(Q_OS_MACOS)
            uid_t uid{};
            gid_t gid{};
            return getpeereid(static_cast<int>(socket->socketDescriptor()), &uid, &gid) == 0 && uid == geteuid();
#else
            Q_UNUSED(socket)
            return true;
#endif
        }

        bool validLength(ControlOpcode opcode, quint32 size, quint32 stateLimit) {
            switch (opcode) {
            case ControlOpcode::Attach:
                return size >= offsetof(AttachRequestV1, statePacket) &&
                       size <= offsetof(AttachRequestV1, statePacket) + stateLimit;
            case ControlOpcode::ConfigureAudio:
                return size == sizeof(ConfigureAudioRequestV1);
            case ControlOpcode::Transport:
                return size == sizeof(TransportRequestV1);
            case ControlOpcode::SetEditorVisible:
                return size == sizeof(SetEditorVisibleRequestV1);
            case ControlOpcode::ProjectAction:
                return size == sizeof(ProjectActionRequestV1);
            case ControlOpcode::RestoreState:
                return size >= BridgeStatePacketHeaderBytesV1 && size <= stateLimit;
            case ControlOpcode::Ping:
            case ControlOpcode::Detach:
            case ControlOpcode::ReleaseAudio:
            case ControlOpcode::BeginOffline:
            case ControlOpcode::EndOffline:
            case ControlOpcode::RevealProject:
            case ControlOpcode::SaveState:
                return size == 0;
            default:
                return false;
            }
        }

        constexpr qsizetype transportBufferSize = 64 * 1024;
        constexpr qsizetype maximumReplyOverhead = ControlFrameHeaderBytesV1 + offsetof(AttachReplyV1, status) +
            offsetof(StatusEventV1, text) + 2 * std::numeric_limits<quint16>::max();

    }

    struct ControlServer::Peer : QObject {
        QLocalSocket *socket{};
        QDeadlineTimer deadline{leaseMilliseconds};
        ControlRequest request{};
        QHash<quint64, quint32> pending;
        quint64 pendingBytes{};
        quint64 lastId{};
        quint32 bodyBytes{};
        quint32 stateLimit{maximumStateBytes};
        bool attachSeen{};
    };

    ControlServer::ControlServer() = default;

    ControlServer::~ControlServer() = default;

    void ControlServer::start() {
        const auto name = endpointName();
        if (name.isEmpty()) {
            qCWarning(lcBridgeControl) << "The audio bridge control directory is unavailable.";
            return;
        }

        m_server = new QLocalServer(this);
        m_server->setSocketOptions(QLocalServer::UserAccessOption);
#ifdef Q_OS_UNIX
        m_endpointLock = std::make_unique<QLockFile>(name + QStringLiteral(".lock"));
        if (!m_endpointLock->tryLock()) {
            qCWarning(lcBridgeControl) << "The audio bridge control endpoint is already locked.";
            return;
        }
        QLocalServer::removeServer(name);
#endif
        if (!m_server->listen(name)) {
            qCWarning(lcBridgeControl) << "Could not listen on the audio bridge control endpoint" << m_server->errorString();
            m_endpointLock.reset();
            return;
        }
        connect(m_server, &QLocalServer::newConnection, this, &ControlServer::acceptPeers);
        auto *leaseTimer = new QTimer(m_server);
        leaseTimer->setInterval(leaseCheckIntervalMs);
        connect(leaseTimer, &QTimer::timeout, this, &ControlServer::expirePeers);
        leaseTimer->start();
    }

    void ControlServer::stop() {
        delete m_server;
        m_server = nullptr;
        for (auto id : m_peers.keys())
            disconnectPeer(id);
        m_endpointLock.reset();
    }

    void ControlServer::acceptPeers() {
        while (auto *socket = m_server->nextPendingConnection()) {
            if (!sameUser(socket)) {
                socket->deleteLater();
                continue;
            }
            auto *peer = new Peer;
            peer->setParent(this);
            peer->socket = socket;
            socket->setParent(peer);
            socket->setReadBufferSize(transportBufferSize);
            const auto id = ++m_nextConnection;
            peer->request.connection = id;
            m_peers.insert(id, peer);
            connect(socket, &QLocalSocket::readyRead, peer, [this, peer] { readPeer(peer); });
            connect(socket, &QLocalSocket::disconnected, peer, [this, id] {
                auto *peer = m_peers.take(id);
                if (peer) {
                    peer->deleteLater();
                    Q_EMIT peerDisconnected(id, static_cast<int>(qMax<qint64>(0, peer->deadline.remainingTime())));
                }
            });
            QMetaObject::invokeMethod(peer, [this, peer] {
                if (m_peers.value(peer->request.connection) == peer)
                    readPeer(peer);
            }, Qt::QueuedConnection);
        }
    }

    void ControlServer::readPeer(Peer *peer) {
        auto *socket = peer->socket;
        while (socket->state() == QLocalSocket::ConnectedState) {
            if (peer->request.id == 0) {
                if (socket->bytesAvailable() < ControlFrameHeaderBytesV1)
                    return;
                const auto header = socket->read(ControlFrameHeaderBytesV1);
                if (header.size() != ControlFrameHeaderBytesV1) {
                    disconnectPeer(peer->request.connection);
                    return;
                }
                const auto bytes = readLE<quint32>(header, offsetof(ControlFrameV1, bodyBytes));
                const auto opcode = static_cast<ControlOpcode>(readLE<quint16>(header, offsetof(ControlFrameV1, opcode)));
                const auto id = readLE<quint64>(header, offsetof(ControlFrameV1, requestId));
                if (id <= peer->lastId || readLE<quint16>(header, offsetof(ControlFrameV1, flags)) != 0) {
                    disconnectPeer(peer->request.connection);
                    return;
                }
                if (!validLength(opcode, bytes, peer->stateLimit) || (!peer->attachSeen && opcode != ControlOpcode::Attach) ||
                    (peer->attachSeen && opcode == ControlOpcode::Attach)) {
                    auto error = ControlErrorCode::InvalidRequest;
                    if ((opcode == ControlOpcode::RestoreState && bytes > peer->stateLimit) ||
                        (opcode == ControlOpcode::Attach && bytes > offsetof(AttachRequestV1, statePacket) + peer->stateLimit))
                        error = ControlErrorCode::StateTooLarge;
                    send(peer->request.connection, opcode, id, quint16(ControlFrameFlag::Reply) | quint16(ControlFrameFlag::Error),
                         errorBody(error, {}));
                    socket->disconnectFromServer();
                    return;
                }
                peer->request.opcode = opcode;
                peer->request.id = id;
                peer->bodyBytes = bytes;
                peer->lastId = id;
            }

            if (peer->request.body.size() < peer->bodyBytes) {
                auto remaining = peer->bodyBytes - peer->request.body.size();
                constexpr qsizetype attachPrefix = offsetof(AttachRequestV1, statePacket);
                if (peer->request.opcode == ControlOpcode::Attach && peer->request.body.size() < attachPrefix)
                    remaining = attachPrefix - peer->request.body.size();
                const auto input = socket->read(remaining);
                if (input.isEmpty())
                    return;
                peer->request.body.append(input);
                peer->deadline = QDeadlineTimer(leaseMilliseconds);

                if (peer->request.opcode == ControlOpcode::Attach && peer->request.body.size() == attachPrefix) {
                    if (readLE<quint16>(peer->request.body, offsetof(AttachRequestV1, protocolVersion)) != BridgeControlVersionV1) {
                        send(peer->request.connection, peer->request.opcode, peer->request.id,
                             quint16(ControlFrameFlag::Reply) | quint16(ControlFrameFlag::Error),
                             errorBody(ControlErrorCode::Incompatible, {}), true);
                        return;
                    }
                    peer->stateLimit = qMin(maximumStateBytes, readLE<quint32>(peer->request.body, offsetof(AttachRequestV1, maxStateBytes)));
                    if (peer->bodyBytes - attachPrefix > peer->stateLimit) {
                        send(peer->request.connection, peer->request.opcode, peer->request.id,
                             quint16(ControlFrameFlag::Reply) | quint16(ControlFrameFlag::Error),
                             errorBody(ControlErrorCode::StateTooLarge, {}));
                        socket->disconnectFromServer();
                        return;
                    }
                }
                if (peer->request.body.size() < peer->bodyBytes)
                    continue;
            }

            auto request = peer->request;
            peer->request.body.clear();
            peer->request.id = 0;
            peer->deadline = QDeadlineTimer(leaseMilliseconds);
            if (request.opcode == ControlOpcode::Ping) {
                send(request.connection, request.opcode, request.id, quint16(ControlFrameFlag::Reply), {});
                continue;
            }
            if (peer->pending.size() == 16 || peer->pendingBytes + request.body.size() > peer->stateLimit + transportBufferSize) {
                disconnectPeer(request.connection);
                return;
            }
            peer->attachSeen = true;
            peer->pending.insert(request.id, static_cast<quint32>(request.body.size()));
            peer->pendingBytes += request.body.size();
            Q_EMIT requestReceived(request);
        }
    }

    void ControlServer::send(quint64 connection, ControlOpcode opcode, quint64 id, quint16 flags, const QByteArray &body, bool close) {
        auto *peer = m_peers.value(connection);
        if (!peer)
            return;
        peer->pendingBytes -= peer->pending.take(id);
        auto *socket = peer->socket;
        if (socket->bytesToWrite() + body.size() > peer->stateLimit + maximumReplyOverhead) {
            disconnectPeer(connection);
            return;
        }
        QByteArray header;
        appendLE(header, static_cast<quint32>(body.size()));
        appendLE(header, static_cast<quint16>(opcode));
        appendLE(header, flags);
        appendLE(header, id);
        if (socket->write(header) != header.size() || (!body.isEmpty() && socket->write(body) != body.size()))
            disconnectPeer(connection);
        else if (close)
            socket->disconnectFromServer();
    }

    void ControlServer::disconnectPeer(quint64 connection) {
        if (auto *peer = m_peers.value(connection))
            peer->socket->abort();
    }

    void ControlServer::expirePeers() {
        for (auto id : m_peers.keys()) {
            if (auto *peer = m_peers.value(id); peer && peer->deadline.hasExpired())
                disconnectPeer(id);
        }
    }

}
