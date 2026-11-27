// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_AUDIOBRIDGESERVER_CONTROLCODEC_H
#define DIFFSCOPE_AUDIOBRIDGESERVER_CONTROLCODEC_H

#include <QByteArray>
#include <QMetaType>
#include <QtEndian>

#include <DiffScopeBridgeProtocol.h>

namespace AudioBridge::Internal {

    inline constexpr quint32 maximumStateBytes = 128 * 1024 * 1024;
    inline constexpr quint32 maximumBlockFrames = 4096;
    inline constexpr int leaseMilliseconds = 5000;

    struct ControlRequest {
        quint64 connection;
        ControlOpcode opcode;
        quint64 id;
        QByteArray body;
    };

    template <typename T>
    T readLE(const QByteArray &bytes, qsizetype offset) {
        return qFromLittleEndian<T>(bytes.constData() + offset);
    }

    template <typename T>
    void appendLE(QByteArray &bytes, T value) {
        char buffer[sizeof(T)];
        qToLittleEndian(value, buffer);
        bytes.append(buffer, sizeof(buffer));
    }

    inline QByteArray errorBody(ControlErrorCode code, const QByteArray &detail) {
        QByteArray body;
        appendLE(body, static_cast<quint16>(code));
        appendLE(body, static_cast<quint16>(detail.size()));
        body.append(detail);
        return body;
    }

}

Q_DECLARE_METATYPE(AudioBridge::Internal::ControlRequest)

#endif
