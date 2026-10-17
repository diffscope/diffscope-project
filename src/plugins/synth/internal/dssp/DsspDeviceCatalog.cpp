// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "DsspDeviceCatalog.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMap>
#include <QProcess>
#include <QTimer>

namespace Synth::Internal {

    DsspDeviceCatalog::DsspDeviceCatalog(QObject *parent) : QObject(parent), m_timer(new QTimer(this)) {
        m_timer->setSingleShot(true);
        m_timer->setInterval(15000);
        connect(m_timer, &QTimer::timeout, this, [this] {
            if (m_process)
                complete(m_process, false, tr("Device discovery timed out."));
        });
    }

    DsspDeviceCatalog::~DsspDeviceCatalog() {
        cancel();
        for (auto process : findChildren<QProcess *>(QString{}, Qt::FindDirectChildrenOnly)) {
            if (process->state() != QProcess::NotRunning) {
                process->kill();
                process->waitForFinished(1000);
            }
        }
    }

    QVariantList DsspDeviceCatalog::providers() const {
        QVariantList result;
        const auto names = QMap<QString, QString>{
            {QStringLiteral("cpu"), QStringLiteral("CPU")},
            {QStringLiteral("cuda"), QStringLiteral("CUDA")},
            {QStringLiteral("directml"), QStringLiteral("DirectML")},
            {QStringLiteral("coreml"), QStringLiteral("CoreML")},
        };
        for (auto it = m_devices.cbegin(); it != m_devices.cend(); ++it) {
            result.append(QVariantMap{
                {QStringLiteral("id"), it.key()},
                {QStringLiteral("name"), names.value(it.key(), it.key())},
            });
        }
        return result;
    }

    QVariantMap DsspDeviceCatalog::devicesByProvider() const {
        return m_devices;
    }

    QVariantMap DsspDeviceCatalog::defaultDevice() const {
        return m_defaultDevice;
    }

    bool DsspDeviceCatalog::busy() const {
        return m_process != nullptr;
    }

    QString DsspDeviceCatalog::errorMessage() const {
        return m_errorMessage;
    }

    QVariantList DsspDeviceCatalog::devices(const QString &provider) const {
        return m_devices.value(provider).toList();
    }

    QVariantMap DsspDeviceCatalog::resolveDevice(const QString &provider, const QString &id) const {
        for (const auto &value : devices(provider)) {
            const auto device = value.toMap();
            if (device.value(QStringLiteral("id")).toString() == id)
                return device;
        }
        return m_defaultDevice;
    }

    void DsspDeviceCatalog::refresh(const QString &executablePath) {
        cancel();
        m_devices.clear();
        m_defaultDevice.clear();
        m_errorMessage.clear();
        if (executablePath.isEmpty()) {
            Q_EMIT changed();
            return;
        }
        auto process = new QProcess(this);
        m_process = process;
        process->setStandardErrorFile(QProcess::nullDevice());
        connect(process, &QProcess::errorOccurred, this, [this, process](QProcess::ProcessError error) {
            if (error == QProcess::FailedToStart)
                complete(process, false, tr("Could not start device discovery."));
        });
        connect(process, &QProcess::finished, this, [this, process](int exitCode, QProcess::ExitStatus status) {
            if (m_process != process)
                return;
            const bool success = status == QProcess::NormalExit && exitCode == 0 && readDevices(process->readAllStandardOutput());
            complete(process, success, success ? QString{} : tr("Could not read execution devices from DiffSinger Synthesis Program."));
        });
        m_timer->start();
        Q_EMIT changed();
        process->start(executablePath, {QStringLiteral("dump-devices")}, QIODevice::ReadOnly);
    }

    void DsspDeviceCatalog::cancel() {
        m_timer->stop();
        if (!m_process)
            return;
        auto process = m_process;
        m_process = nullptr;
        process->disconnect(this);
        if (process->state() != QProcess::NotRunning) {
            connect(process, &QProcess::finished, process, &QObject::deleteLater);
            process->kill();
        } else {
            process->deleteLater();
        }
        Q_EMIT changed();
    }

    void DsspDeviceCatalog::complete(QProcess *process, bool success, const QString &errorMessage) {
        if (m_process != process)
            return;
        cancel();
        m_errorMessage = errorMessage;
        if (!success) {
            m_devices.clear();
            m_defaultDevice.clear();
        }
        Q_EMIT changed();
        Q_EMIT finished(success);
    }

    bool DsspDeviceCatalog::readDevices(const QByteArray &json) {
        const auto document = QJsonDocument::fromJson(json);
        if (!document.isObject())
            return false;
        const auto root = document.object();
        const auto groups = root.value(QStringLiteral("devices")).toObject();
        const auto defaultDevice = root.value(QStringLiteral("default_device")).toObject();
        QVariantMap devices;
        for (auto it = groups.begin(); it != groups.end(); ++it) {
            QVariantList list;
            for (const auto &value : it.value().toArray()) {
                const auto object = value.toObject();
                if (!object.value(QStringLiteral("id")).isString() || object.value(QStringLiteral("id")).toString().isEmpty() ||
                    !object.value(QStringLiteral("description")).isString() || !object.value(QStringLiteral("index")).isDouble() ||
                    object.value(QStringLiteral("index")).toInt(-1) < 0)
                    return false;
                auto device = object.toVariantMap();
                device.insert(QStringLiteral("executionProvider"), it.key());
                list.append(device);
            }
            if (!list.isEmpty())
                devices.insert(it.key(), list);
        }
        const auto provider = defaultDevice.value(QStringLiteral("execution_provider")).toString();
        const auto id = defaultDevice.value(QStringLiteral("id")).toString();
        for (const auto &value : devices.value(provider).toList()) {
            const auto device = value.toMap();
            if (device.value(QStringLiteral("id")).toString() == id) {
                m_devices = devices;
                m_defaultDevice = device;
                return true;
            }
        }
        return false;
    }

}

#include "moc_DsspDeviceCatalog.cpp"
