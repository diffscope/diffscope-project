// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_AUDIO_EFFECTSEXPORTLISTENER_H
#define DIFFSCOPE_AUDIO_EFFECTSEXPORTLISTENER_H

#include <QHash>
#include <QMutex>
#include <QPointer>

#include <audio/AudioExporter.h>

namespace Audio::Internal {

    class EffectsAddOn;

    class EffectsExportListener final : public AudioExporterListener {
    public:
        static EffectsExportListener &instance();

        bool willStartCallback(AudioExporter *exporter) override;
        void willFinishCallback(AudioExporter *exporter) override;

    private:
        EffectsExportListener();

        static bool setBypassActive(const QPointer<EffectsAddOn> &addOn, bool active);

        QMutex m_mutex;
        QHash<AudioExporter *, QPointer<EffectsAddOn>> m_bypassedAddOns;
    };

}

#endif // DIFFSCOPE_AUDIO_EFFECTSEXPORTLISTENER_H
