// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "BridgeServer.h"

#include <audiobridgeserver/internal/AudioIpc.h>
#include <audiobridgeserver/internal/BridgeSession.h>
#include <audiobridgeserver/internal/ControlServer.h>

namespace AudioBridge::Internal {

    namespace {
        constexpr auto replyFlag = quint16(ControlFrameFlag::Reply);
        constexpr auto errorFlag = replyFlag | quint16(ControlFrameFlag::Error);
    }

    BridgeServer::BridgeServer(QObject *parent)
        : QObject(parent), m_audio(std::make_unique<AudioIpc>()), m_control(new ControlServer) {
        qRegisterMetaType<ControlRequest>();
        m_control->moveToThread(&m_controlThread);
        connect(&m_controlThread, &QThread::started, m_control, &ControlServer::start);
        connect(&m_controlThread, &QThread::finished, m_control, &QObject::deleteLater);
        connect(m_control, &ControlServer::requestReceived, this, &BridgeServer::receive);
        connect(m_control, &ControlServer::peerDisconnected, this, &BridgeServer::disconnected);
    }

    BridgeServer::~BridgeServer() {
        stop();
    }

    void BridgeServer::start() {
        m_controlThread.setObjectName(QStringLiteral("Audio Bridge Control"));
        m_controlThread.start();
    }

    void BridgeServer::stop() {
        if (m_stopping)
            return;
        m_stopping = true;
        if (m_controlThread.isRunning()) {
            QMetaObject::invokeMethod(m_control, &ControlServer::stop, Qt::BlockingQueuedConnection);
            m_controlThread.quit();
            m_controlThread.wait();
        } else {
            delete m_control;
        }
        m_control = nullptr;
        m_sessions.clear();
        m_connections.clear();
        for (auto *child : children()) {
            if (auto *session = dynamic_cast<BridgeSession *>(child))
                session->close(true, true);
        }
        m_audio.reset();
    }

    void BridgeServer::receive(const ControlRequest &request) {
        if (m_stopping)
            return;
        if (request.opcode == ControlOpcode::Attach) {
            attach(request);
            return;
        }
        auto *session = m_connections.value(request.connection);
        if (!session) {
            error(request, ControlErrorCode::InvalidState);
            return;
        }
        if (request.opcode == ControlOpcode::Detach) {
            send(request.connection, request.opcode, request.id, replyFlag, {}, true);
            closeSession(session, false);
        } else {
            session->dispatch(request);
        }
    }

    void BridgeServer::attach(const ControlRequest &request) {
        const auto &body = request.body;
        if (readLE<quint16>(body, offsetof(AttachRequestV1, protocolVersion)) != BridgeControlVersionV1) {
            error(request, ControlErrorCode::Incompatible, {}, true);
            return;
        }
        const auto kind = static_cast<BridgeStateKind>(body.at(offsetof(AttachRequestV1, initialKind)));
        const auto visible = quint8(body.at(offsetof(AttachRequestV1, editorVisible)));
        const auto limit = qMin(maximumStateBytes, readLE<quint32>(body, offsetof(AttachRequestV1, maxStateBytes)));
        if ((kind != BridgeStateKind::ExternalFile && kind != BridgeStateKind::HostState) || visible > 1 ||
            limit < BridgeStatePacketHeaderBytesV1 || readLE<quint16>(body, offsetof(AttachRequestV1, reserved)) != 0) {
            error(request, ControlErrorCode::InvalidRequest, {}, true);
            return;
        }
        const auto instanceId = body.first(sizeof(AttachRequestV1::instanceId));
        auto *session = m_sessions.value(instanceId);
        const bool resumed = session;
        if (session && !session->canAttach()) {
            error(request, ControlErrorCode::Busy, {}, true);
            return;
        }
        if (!session) {
            session = new BridgeSession(this, m_audio.get(), instanceId);
            if (!session->initialize(kind)) {
                session->deleteLater();
                error(request, ControlErrorCode::LoadFailed, {}, true);
                return;
            }
            m_sessions.insert(instanceId, session);
            session->lease.setSingleShot(true);
            connect(&session->lease, &QTimer::timeout, session, [this, session] { closeSession(session, true); });
        }
        m_connections.insert(request.connection, session);
        const bool audioCompatible = readLE<quint16>(body, offsetof(AttachRequestV1, audioAbi)) == BridgeAudioAbiV1;
        const auto packet = resumed ? QByteArray{} : body.sliced(offsetof(AttachRequestV1, statePacket));
        session->attach(request.connection, limit, audioCompatible, visible != 0, packet);

        if (session->connection() != request.connection)
            return;
        QByteArray response;
        appendLE(response, limit);
        appendLE(response, quint8(resumed));
        response.append(3, '\0');
        response.append(session->statusBody());
        reply(request, response);
        status(request.connection, session->statusBody());
    }

    void BridgeServer::disconnected(quint64 connection, int remainingLease) {
        if (auto *session = m_connections.take(connection))
            session->disconnect(remainingLease);
    }

    void BridgeServer::closeSession(BridgeSession *session, bool preserveRecoveryLog) {
        m_sessions.remove(session->instanceId);
        m_connections.remove(session->connection());
        session->close(preserveRecoveryLog);
    }

    void BridgeServer::send(quint64 connection, ControlOpcode opcode, quint64 id, quint16 flags,
                            const QByteArray &body, bool close) {
        if (m_stopping || !connection)
            return;
        auto *control = m_control;
        QMetaObject::invokeMethod(control, [control, connection, opcode, id, flags, body, close] {
            control->send(connection, opcode, id, flags, body, close);
        }, Qt::QueuedConnection);
    }

    void BridgeServer::reply(const ControlRequest &request, const QByteArray &body) {
        send(request.connection, request.opcode, request.id, replyFlag, body);
    }

    void BridgeServer::error(const ControlRequest &request, ControlErrorCode code, const QString &detail, bool close) {
        send(request.connection, request.opcode, request.id, errorFlag, errorBody(code, detail.left(20000).toUtf8()), close);
    }

    void BridgeServer::status(quint64 connection, const QByteArray &body) {
        send(connection, ControlOpcode::Status, BridgeEventRequestId, quint16(ControlFrameFlag::Event), body);
    }

}
