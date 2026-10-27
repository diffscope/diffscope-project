// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_AUDIO_MIDIINPUTCONTEXT_H
#define DIFFSCOPE_AUDIO_MIDIINPUTCONTEXT_H

#include <QObject>
#include <QScopedPointer>
#include <QString>
#include <QVariantList>

#include <TalcsMidi/MidiInputEvent.h>

#include <audio/audioglobal.h>

namespace talcs {
    class MidiMessageListener;
}

namespace Audio {

    class MidiInputContextPrivate;

    class AUDIO_EXPORT MidiInputContext : public QObject {
        Q_OBJECT
        Q_DECLARE_PRIVATE(MidiInputContext)
        Q_PROPERTY(QVariantList devices READ devices NOTIFY devicesChanged)

    public:
        explicit MidiInputContext(QObject *parent = nullptr);
        ~MidiInputContext() override;

        static MidiInputContext *instance();
        void initialize();

        QVariantList devices() const;
        QString deviceName() const;
        bool isDeviceReady() const;
        QString deviceErrorString() const;
        QString deviceId() const;
        bool setDeviceId(const QString &deviceId);
        bool previewEnabled() const;
        void setPreviewEnabled(bool enabled);
        QString previewType() const;
        void setPreviewType(const QString &type);
        double previewGain() const;
        void setPreviewGain(double gain);
        bool mmcEnabled() const;
        void setMmcEnabled(bool enabled);

        bool playPreviewTest(bool alternative = false);
        void stopPreviewTest();
        bool isPreviewTestPlaying() const;

        Q_INVOKABLE void refreshDevices();
        Q_INVOKABLE void interruptPreview();

        /// Returns the consuming input dispatcher. Observers must return false and unregister before destruction.
        talcs::MidiMessageListener *listener() const;
        /// Dispatches input from any thread. Callers must stop submitting before this context is destroyed.
        /// Events use a distinct source ID and the steady clock specified by MidiInputEvent.
        void submitEvent(const talcs::MidiInputEvent &event);

    Q_SIGNALS:
        void devicesChanged();
        void deviceChanged();
        void previewEnabledChanged();
        void previewTypeChanged();
        void previewGainChanged();
        void mmcEnabledChanged();

    private:
        QScopedPointer<MidiInputContextPrivate> d_ptr;
    };

}

#endif // DIFFSCOPE_AUDIO_MIDIINPUTCONTEXT_H
