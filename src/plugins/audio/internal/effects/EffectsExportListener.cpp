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
        if (!exporter || exporter->config().isEffectsEnabled()) {
            return true;
        }
        QPointer<EffectsAddOn> addOn = EffectsAddOn::of(exporter->windowHandle());
        if (!addOn) {
            return true;
        }
        if (!setBypassActive(addOn, true)) {
            return false;
        }
        QMutexLocker locker(&m_mutex);
        m_bypassedAddOns.insert(exporter, addOn);
        return true;
    }

    void EffectsExportListener::willFinishCallback(AudioExporter *exporter) {
        QPointer<EffectsAddOn> addOn;
        {
            QMutexLocker locker(&m_mutex);
            addOn = m_bypassedAddOns.take(exporter);
        }
        if (addOn) {
            setBypassActive(addOn, false);
        }
    }

    bool EffectsExportListener::setBypassActive(const QPointer<EffectsAddOn> &addOn, bool active) {
        if (!addOn) {
            return false;
        }
        const auto apply = [addOn, active] {
            if (!addOn) {
                return;
            }
            if (active) {
                addOn->beginEffectsBypass();
            } else {
                addOn->endEffectsBypass();
            }
        };
        if (QThread::currentThread() == addOn->thread()) {
            apply();
            return true;
        }
        return QMetaObject::invokeMethod(addOn, apply, Qt::BlockingQueuedConnection);
    }

}
