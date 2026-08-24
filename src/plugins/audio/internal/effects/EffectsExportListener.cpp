// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "EffectsExportListener.h"

#include <QMetaObject>
#include <QMutexLocker>
#include <QThread>

#include <audio/AudioExporterConfig.h>
#include <audio/internal/EffectsAddOn.h>

namespace Audio::Internal {

    EffectsExportListener &EffectsExportListener::instance() {
        static EffectsExportListener listener;
        return listener;
    }

    EffectsExportListener::EffectsExportListener() {
        AudioExporter::registerListener(this);
    }

    bool EffectsExportListener::willStartCallback(AudioExporter *exporter) {
        if (!exporter) {
            return true;
        }
        QPointer<EffectsAddOn> addOn = EffectsAddOn::of(exporter->windowHandle());
        if (!addOn) {
            return true;
        }
        const bool bypassed = !exporter->config().isEffectsEnabled();
        if (!updateEffectsForExport(addOn, true, bypassed)) {
            return false;
        }
        QMutexLocker locker(&m_mutex);
        m_exportStates.insert(exporter, {addOn, bypassed});
        return true;
    }

    void EffectsExportListener::willFinishCallback(AudioExporter *exporter) {
        ExportState state;
        {
            QMutexLocker locker(&m_mutex);
            state = m_exportStates.take(exporter);
        }
        if (state.addOn) {
            updateEffectsForExport(state.addOn, false, state.bypassed);
        }
    }

    bool EffectsExportListener::updateEffectsForExport(const QPointer<EffectsAddOn> &addOn, bool starting, bool bypassed) {
        if (!addOn) {
            return false;
        }
        const auto apply = [addOn, starting, bypassed] {
            if (!addOn) {
                return;
            }
            if (!starting && bypassed) {
                addOn->endEffectsBypass();
            }
            addOn->refreshAllEffects();
            if (starting && bypassed) {
                addOn->beginEffectsBypass();
            }
        };
        if (QThread::currentThread() == addOn->thread()) {
            apply();
            return true;
        }
        return QMetaObject::invokeMethod(addOn, apply, Qt::BlockingQueuedConnection);
    }

}
