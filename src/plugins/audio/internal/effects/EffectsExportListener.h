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
        struct ExportState {
            QPointer<EffectsAddOn> addOn;
            bool bypassed{};
        };

        EffectsExportListener();

        static bool updateEffectsForExport(const QPointer<EffectsAddOn> &addOn, bool starting, bool bypassed);

        QMutex m_mutex;
        QHash<AudioExporter *, ExportState> m_exportStates;
    };

}

#endif // DIFFSCOPE_AUDIO_EFFECTSEXPORTLISTENER_H
