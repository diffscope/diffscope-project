// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "SynthesisVisualizerSettings.h"

#include <QQmlEngine>
#include <QSettings>

#include <CoreApi/runtimeinterface.h>

namespace SynthVisualizer::Internal {

    SynthesisVisualizerSettings *SynthesisVisualizerSettings::m_instance = nullptr;

    SynthesisVisualizerSettings::SynthesisVisualizerSettings(QObject *parent) : QObject(parent) {
        Q_ASSERT(!m_instance);
        m_instance = this;
        auto settings = Core::RuntimeInterface::settings();
        settings->beginGroup(QStringLiteral("org.diffscope.synthvisualizer.visualization"));
        m_showPhonemeWaveforms = settings->value(QStringLiteral("showPhonemeWaveforms"), true).toBool();
        m_showClipWaveforms = settings->value(QStringLiteral("showClipWaveforms"), false).toBool();
        settings->endGroup();
    }

    SynthesisVisualizerSettings::~SynthesisVisualizerSettings() {
        m_instance = nullptr;
    }

    SynthesisVisualizerSettings *SynthesisVisualizerSettings::instance() {
        return m_instance;
    }

    SynthesisVisualizerSettings *SynthesisVisualizerSettings::create(QQmlEngine *, QJSEngine *) {
        Q_ASSERT(m_instance);
        QQmlEngine::setObjectOwnership(m_instance, QQmlEngine::CppOwnership);
        return m_instance;
    }

    bool SynthesisVisualizerSettings::showPhonemeWaveforms() const {
        return m_showPhonemeWaveforms;
    }

    bool SynthesisVisualizerSettings::showClipWaveforms() const {
        return m_showClipWaveforms;
    }

    void SynthesisVisualizerSettings::setOptions(bool showPhonemeWaveforms, bool showClipWaveforms) {
        const bool phonemeWaveformsChanged = m_showPhonemeWaveforms != showPhonemeWaveforms;
        const bool clipWaveformsChanged = m_showClipWaveforms != showClipWaveforms;
        m_showPhonemeWaveforms = showPhonemeWaveforms;
        m_showClipWaveforms = showClipWaveforms;

        auto settings = Core::RuntimeInterface::settings();
        settings->beginGroup(QStringLiteral("org.diffscope.synthvisualizer.visualization"));
        settings->setValue(QStringLiteral("showPhonemeWaveforms"), showPhonemeWaveforms);
        settings->setValue(QStringLiteral("showClipWaveforms"), showClipWaveforms);
        settings->endGroup();

        if (phonemeWaveformsChanged)
            Q_EMIT showPhonemeWaveformsChanged();
        if (clipWaveformsChanged)
            Q_EMIT showClipWaveformsChanged();
    }

}

#include "moc_SynthesisVisualizerSettings.cpp"
