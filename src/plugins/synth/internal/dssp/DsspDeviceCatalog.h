// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_SYNTH_INTERNAL_DSSPDEVICECATALOG_H
#define DIFFSCOPE_SYNTH_INTERNAL_DSSPDEVICECATALOG_H

#include <QByteArray>
#include <QObject>
#include <QVariant>

class QProcess;
class QTimer;

namespace Synth::Internal {

    class DsspDeviceCatalog : public QObject {
        Q_OBJECT
        Q_PROPERTY(QVariantList providers READ providers NOTIFY changed)
        Q_PROPERTY(QVariantMap devicesByProvider READ devicesByProvider NOTIFY changed)
        Q_PROPERTY(QVariantMap defaultDevice READ defaultDevice NOTIFY changed)
        Q_PROPERTY(bool busy READ busy NOTIFY changed)
        Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY changed)
    public:
        explicit DsspDeviceCatalog(QObject *parent = nullptr);
        ~DsspDeviceCatalog() override;

        QVariantList providers() const;
        QVariantMap devicesByProvider() const;
        QVariantMap defaultDevice() const;
        bool busy() const;
        QString errorMessage() const;
        QVariantList devices(const QString &provider) const;
        QVariantMap resolveDevice(const QString &provider, const QString &id) const;

        void refresh(const QString &executablePath);
        void cancel();

    Q_SIGNALS:
        void changed();
        void finished(bool success);

    private:
        void complete(QProcess *process, bool success, const QString &errorMessage = {});
        bool readDevices(const QByteArray &json);

        QProcess *m_process{};
        QTimer *m_timer{};
        QVariantMap m_devices;
        QVariantMap m_defaultDevice;
        QString m_errorMessage;
    };

}

#endif // DIFFSCOPE_SYNTH_INTERNAL_DSSPDEVICECATALOG_H
