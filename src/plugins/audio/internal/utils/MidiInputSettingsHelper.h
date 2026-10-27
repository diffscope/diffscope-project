// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_AUDIO_MIDIINPUTSETTINGSHELPER_H
#define DIFFSCOPE_AUDIO_MIDIINPUTSETTINGSHELPER_H

#include <QObject>
#include <QPointer>
#include <QString>
#include <QStringList>
#include <qqmlintegration.h>

class QTimer;

namespace Audio {
    class MidiInputContext;
}

namespace Audio::Internal {

    class MidiInputSettingsHelper : public QObject {
        Q_OBJECT
        QML_ELEMENT
        Q_PROPERTY(QStringList deviceList READ deviceList NOTIFY deviceListChanged)
        Q_PROPERTY(int deviceCurrentIndex READ deviceCurrentIndex WRITE setDeviceCurrentIndex NOTIFY deviceCurrentIndexChanged)
        Q_PROPERTY(bool previewEnabled READ previewEnabled WRITE setPreviewEnabled NOTIFY previewEnabledChanged)
        Q_PROPERTY(QStringList previewTypeList READ previewTypeList CONSTANT)
        Q_PROPERTY(int previewTypeCurrentIndex READ previewTypeCurrentIndex WRITE setPreviewTypeCurrentIndex NOTIFY previewTypeCurrentIndexChanged)
        Q_PROPERTY(double previewGain READ previewGain WRITE setPreviewGain NOTIFY previewGainChanged)
        Q_PROPERTY(bool mmcEnabled READ mmcEnabled WRITE setMmcEnabled NOTIFY mmcEnabledChanged)
        Q_PROPERTY(bool previewTestPlaying READ previewTestPlaying WRITE setPreviewTestPlaying NOTIFY previewTestPlayingChanged)
        Q_PROPERTY(QString previewTestToolTip READ previewTestToolTip NOTIFY previewTestToolTipChanged)
        Q_PROPERTY(int previewTestInputLength READ previewTestInputLength NOTIFY previewTestInputLengthChanged)

    public:
        explicit MidiInputSettingsHelper(QObject *parent = nullptr);
        ~MidiInputSettingsHelper() override;

        QStringList deviceList() const;
        int deviceCurrentIndex() const;
        void setDeviceCurrentIndex(int index);
        bool previewEnabled() const;
        void setPreviewEnabled(bool enabled);
        QStringList previewTypeList() const;
        int previewTypeCurrentIndex() const;
        void setPreviewTypeCurrentIndex(int index);
        double previewGain() const;
        void setPreviewGain(double gain);
        bool mmcEnabled() const;
        void setMmcEnabled(bool enabled);
        bool previewTestPlaying() const;
        void setPreviewTestPlaying(bool playing);
        QString previewTestToolTip() const;
        int previewTestInputLength() const;

        Q_INVOKABLE void interruptPreview();
        Q_INVOKABLE void inputPreviewTestText(const QString &text);
        Q_INVOKABLE void finishPreviewTestInput();

    Q_SIGNALS:
        void deviceListChanged();
        void deviceCurrentIndexChanged();
        void previewEnabledChanged();
        void previewTypeCurrentIndexChanged();
        void previewGainChanged();
        void mmcEnabledChanged();
        void previewTestPlayingChanged();
        void previewTestToolTipChanged();
        void previewTestInputLengthChanged();
        void deviceError(const QString &message);

    private:
        void updateDeviceList();
        void finishPreviewTest();

        static bool m_easterEggEnabled;

        QPointer<MidiInputContext> m_context;
        QStringList m_deviceIds;
        QStringList m_deviceList;
        int m_deviceCurrentIndex{};
        QTimer *m_previewTestTimer{};
        bool m_previewTestPlaying{};
        QString m_previewTestToolTip;
        int m_easterEggInputLength{};
    };

}

#endif // DIFFSCOPE_AUDIO_MIDIINPUTSETTINGSHELPER_H
