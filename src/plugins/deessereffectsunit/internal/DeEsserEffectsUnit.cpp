// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "DeEsserEffectsUnit.h"

#include <algorithm>
#include <cmath>
#include <memory>

#include <QJsonObject>
#include <QQmlComponent>
#include <QQuickItem>
#include <QTimer>
#include <QVariant>

#include <CoreApi/runtimeinterface.h>

#include <deessereffectsunit/internal/DeEsserProcessor.h>

namespace DeEsserEffectsUnit::Internal {

    namespace {

        constexpr int analysisRefreshInterval = 8;
        constexpr int analysisIdleTimeout = 250;
        constexpr int levelMeterRampLength = 128;
        constexpr int spectrumRampLength = 64;

        bool valuesEqual(double left, double right) {
            return qFuzzyIsNull(left - right);
        }

        double normalizedValue(const QJsonValue &value, double fallback,
                               double minimum, double maximum) {
            if (!value.isDouble() || !std::isfinite(value.toDouble())) {
                return fallback;
            }
            return std::clamp(value.toDouble(), minimum, maximum);
        }

        bool normalizedBoolean(const QJsonValue &value, bool fallback) {
            return value.isBool() ? value.toBool() : fallback;
        }

        void updateSmoothedValue(talcs::SmoothedFloat &smoothedValue, float value) {
            if (value < smoothedValue.currentValue()) {
                smoothedValue.setTargetValue(value);
            } else {
                smoothedValue.setCurrentAndTargetValue(value);
            }
        }

    }

    DeEsserEffectsUnit::DeEsserEffectsUnit(QQmlComponent *editorComponent,
                                           QObject *parent)
        : EffectsUnit(parent),
          m_leftBandLevel(meterFloorDb),
          m_rightBandLevel(meterFloorDb),
          m_leftGainReduction(0.0f),
          m_rightGainReduction(0.0f),
          m_committedFrequencyHz(defaultFrequencyHz),
          m_committedBandwidthHz(defaultBandwidthHz),
          m_committedThresholdDb(defaultThresholdDb),
          m_frequencyHz(defaultFrequencyHz),
          m_bandwidthHz(defaultBandwidthHz),
          m_thresholdDb(defaultThresholdDb),
          m_outputSibilanceOnly(defaultOutputSibilanceOnly) {
        auto processor = std::make_unique<DeEsserProcessor>();
        m_processor = processor.get();
        setProcessor(std::move(processor));
        updateProcessor();

        m_leftBandLevel.setRampLength(levelMeterRampLength);
        m_rightBandLevel.setRampLength(levelMeterRampLength);
        m_leftGainReduction.setRampLength(levelMeterRampLength);
        m_rightGainReduction.setRampLength(levelMeterRampLength);
        m_spectrumCurve.fill(meterFloorDb);
        for (auto &value : m_smoothedSpectrum) {
            value.setCurrentAndTargetValue(meterFloorDb);
            value.setRampLength(spectrumRampLength);
        }

        m_analysisTimer = new QTimer(this);
        m_analysisTimer->setSingleShot(true);
        connect(m_analysisTimer, &QTimer::timeout,
                this, &DeEsserEffectsUnit::tickAnalysis);

        auto object = editorComponent->createWithInitialProperties({
            {QStringLiteral("effectsUnit"), QVariant::fromValue(this)},
        }, editorComponent->creationContext());
        if (!object) {
            qFatal() << editorComponent->errorString();
        }
        auto editor = qobject_cast<QQuickItem *>(object);
        if (!editor) {
            delete object;
            qFatal("DeEsserEditor must create a QQuickItem");
        }
        setEditor(editor);
        connect(editor, &QQuickItem::windowChanged, this,
                [this] { updateAnalysisTimer(); });
        connect(editor, &QQuickItem::visibleChanged, this,
                [this] { updateAnalysisTimer(); });
        updateAnalysisTimer();
    }

    DeEsserEffectsUnit::~DeEsserEffectsUnit() {
        stopAnalysisTimer();
    }

    double DeEsserEffectsUnit::frequencyHz() const {
        return m_frequencyHz;
    }

    double DeEsserEffectsUnit::bandwidthHz() const {
        return m_bandwidthHz;
    }

    double DeEsserEffectsUnit::thresholdDb() const {
        return m_thresholdDb;
    }

    bool DeEsserEffectsUnit::outputSibilanceOnly() const {
        return m_outputSibilanceOnly;
    }

    double DeEsserEffectsUnit::leftBandLevelDb() const {
        return m_leftBandLevel.currentValue();
    }

    double DeEsserEffectsUnit::rightBandLevelDb() const {
        return m_rightBandLevel.currentValue();
    }

    double DeEsserEffectsUnit::leftGainReductionDb() const {
        return m_leftGainReduction.currentValue();
    }

    double DeEsserEffectsUnit::rightGainReductionDb() const {
        return m_rightGainReduction.currentValue();
    }

    QJsonValue DeEsserEffectsUnit::getState() const {
        return QJsonObject{
            {QStringLiteral("frequencyHz"), m_committedFrequencyHz},
            {QStringLiteral("bandwidthHz"), m_committedBandwidthHz},
            {QStringLiteral("thresholdDb"), m_committedThresholdDb},
            {QStringLiteral("outputSibilanceOnly"), m_outputSibilanceOnly},
        };
    }

    void DeEsserEffectsUnit::setState(const QJsonValue &state) {
        const auto object = state.isObject() ? state.toObject() : QJsonObject{};
        const double frequency = normalizedValue(
            object.value(QStringLiteral("frequencyHz")), defaultFrequencyHz,
            minimumFrequencyHz, maximumFrequencyHz);
        const double bandwidth = normalizedValue(
            object.value(QStringLiteral("bandwidthHz")), defaultBandwidthHz,
            minimumBandwidthHz, maximumBandwidthHz);
        const double threshold = normalizedValue(
            object.value(QStringLiteral("thresholdDb")), defaultThresholdDb,
            minimumThresholdDb, maximumThresholdDb);
        const bool outputOnly = normalizedBoolean(
            object.value(QStringLiteral("outputSibilanceOnly")),
            defaultOutputSibilanceOnly);

        const bool stateChanged = !valuesEqual(m_committedFrequencyHz, frequency)
            || !valuesEqual(m_committedBandwidthHz, bandwidth)
            || !valuesEqual(m_committedThresholdDb, threshold)
            || m_outputSibilanceOnly != outputOnly;
        const bool frequencyChanged = !valuesEqual(m_frequencyHz, frequency);
        const bool bandwidthChanged = !valuesEqual(m_bandwidthHz, bandwidth);
        const bool thresholdChanged = !valuesEqual(m_thresholdDb, threshold);
        const bool outputOnlyChanged = m_outputSibilanceOnly != outputOnly;

        m_committedFrequencyHz = frequency;
        m_committedBandwidthHz = bandwidth;
        m_committedThresholdDb = threshold;
        m_frequencyHz = frequency;
        m_bandwidthHz = bandwidth;
        m_thresholdDb = threshold;
        m_outputSibilanceOnly = outputOnly;
        updateProcessor();

        if (frequencyChanged) {
            Q_EMIT frequencyHzChanged();
        }
        if (bandwidthChanged) {
            Q_EMIT bandwidthHzChanged();
        }
        if (thresholdChanged) {
            Q_EMIT thresholdDbChanged();
        }
        if (outputOnlyChanged) {
            Q_EMIT outputSibilanceOnlyChanged();
        }
        if (stateChanged) {
            Q_EMIT updated();
        }
    }

    void DeEsserEffectsUnit::refresh() {
        m_processor->refresh();
        m_processor->discardMeterValues();
        m_processor->discardSpectrumFrames();
        resetLevelDisplays();
        resetSpectrumDisplay();
        m_meterDecayStarted = false;
        m_spectrumDecayStarted = false;
        if (m_analysisActive) {
            m_lastMeterValueTime.restart();
            m_lastSpectrumFrameTime.restart();
        }
    }

    void DeEsserEffectsUnit::previewFrequencyHz(double value) {
        if (previewValue(m_frequencyHz, value,
                         minimumFrequencyHz, maximumFrequencyHz)) {
            Q_EMIT frequencyHzChanged();
        }
    }

    void DeEsserEffectsUnit::previewBandwidthHz(double value) {
        if (previewValue(m_bandwidthHz, value,
                         minimumBandwidthHz, maximumBandwidthHz)) {
            Q_EMIT bandwidthHzChanged();
        }
    }

    void DeEsserEffectsUnit::previewThresholdDb(double value) {
        if (previewValue(m_thresholdDb, value,
                         minimumThresholdDb, maximumThresholdDb)) {
            Q_EMIT thresholdDbChanged();
        }
    }

    void DeEsserEffectsUnit::previewBandEdges(double leftFrequencyHz,
                                              double rightFrequencyHz) {
        if (!std::isfinite(leftFrequencyHz) || !std::isfinite(rightFrequencyHz)) {
            return;
        }
        const double left = std::clamp(
            std::min(leftFrequencyHz, rightFrequencyHz),
            minimumFrequencyHz, maximumFrequencyHz);
        const double right = std::clamp(
            std::max(leftFrequencyHz, rightFrequencyHz),
            minimumFrequencyHz, maximumFrequencyHz);
        const double bandwidth = std::clamp(
            right - left, minimumBandwidthHz, maximumBandwidthHz);
        const double frequency = std::clamp(
            (left + right) * 0.5, minimumFrequencyHz, maximumFrequencyHz);
        const bool frequencyChanged = !valuesEqual(m_frequencyHz, frequency);
        const bool bandwidthChanged = !valuesEqual(m_bandwidthHz, bandwidth);
        if (!frequencyChanged && !bandwidthChanged) {
            return;
        }
        m_frequencyHz = frequency;
        m_bandwidthHz = bandwidth;
        updateProcessor();
        if (frequencyChanged) {
            Q_EMIT frequencyHzChanged();
        }
        if (bandwidthChanged) {
            Q_EMIT bandwidthHzChanged();
        }
    }

    void DeEsserEffectsUnit::commitPreview() {
        if (valuesEqual(m_committedFrequencyHz, m_frequencyHz)
            && valuesEqual(m_committedBandwidthHz, m_bandwidthHz)
            && valuesEqual(m_committedThresholdDb, m_thresholdDb)) {
            return;
        }
        m_committedFrequencyHz = m_frequencyHz;
        m_committedBandwidthHz = m_bandwidthHz;
        m_committedThresholdDb = m_thresholdDb;
        Q_EMIT updated();
    }

    void DeEsserEffectsUnit::setFrequencyHz(double value) {
        previewFrequencyHz(value);
        commitPreview();
    }

    void DeEsserEffectsUnit::setBandwidthHz(double value) {
        previewBandwidthHz(value);
        commitPreview();
    }

    void DeEsserEffectsUnit::setThresholdDb(double value) {
        previewThresholdDb(value);
        commitPreview();
    }

    void DeEsserEffectsUnit::setOutputSibilanceOnly(bool enabled) {
        if (m_outputSibilanceOnly == enabled) {
            return;
        }
        m_outputSibilanceOnly = enabled;
        updateProcessor();
        Q_EMIT outputSibilanceOnlyChanged();
        Q_EMIT updated();
    }

    const std::array<float, deEsserSpectrumBinCount> &
        DeEsserEffectsUnit::spectrumCurveDb() const {
        return m_spectrumCurve;
    }

    bool DeEsserEffectsUnit::previewValue(double &member, double value,
                                          double minimum, double maximum) {
        if (!std::isfinite(value)) {
            return false;
        }
        value = std::clamp(value, minimum, maximum);
        if (valuesEqual(member, value)) {
            return false;
        }
        member = value;
        updateProcessor();
        return true;
    }

    void DeEsserEffectsUnit::updateProcessor() {
        m_processor->setParameters(m_frequencyHz, m_bandwidthHz, m_thresholdDb,
                                   m_outputSibilanceOnly);
    }

    void DeEsserEffectsUnit::updateAnalysisTimer() {
        const bool shouldRun = editor() && editor()->window() && editor()->isVisible();
        if (shouldRun) {
            startAnalysisTimer();
        } else {
            stopAnalysisTimer();
        }
    }

    void DeEsserEffectsUnit::startAnalysisTimer() {
        if (m_analysisActive) {
            return;
        }
        m_processor->discardMeterValues();
        m_processor->discardSpectrumFrames();
        m_processor->setAnalysisEnabled(true);
        resetLevelDisplays();
        resetSpectrumDisplay();
        m_analysisActive = true;
        m_meterDecayStarted = false;
        m_spectrumDecayStarted = false;
        m_analysisTickTime.start();
        m_lastMeterValueTime.start();
        m_lastSpectrumFrameTime.start();
        tickAnalysis();
    }

    void DeEsserEffectsUnit::stopAnalysisTimer() {
        m_processor->setAnalysisEnabled(false);
        if (!m_analysisActive) {
            return;
        }
        m_analysisActive = false;
        m_analysisTimer->stop();
        m_processor->discardMeterValues();
        m_processor->discardSpectrumFrames();
        resetLevelDisplays();
        resetSpectrumDisplay();
    }

    void DeEsserEffectsUnit::tickAnalysis() {
        if (!m_analysisActive) {
            return;
        }
        if (!editor() || !editor()->window() || !editor()->isVisible()) {
            stopAnalysisTimer();
            return;
        }

        DeEsserProcessor::MeterValues meterValues;
        bool receivedMeterValues = false;
        while (m_processor->takeMeterValues(meterValues)) {
            receivedMeterValues = true;
            updateSmoothedValue(m_leftBandLevel, meterValues.bandLevelDb[0]);
            updateSmoothedValue(m_rightBandLevel, meterValues.bandLevelDb[1]);
            updateSmoothedValue(m_leftGainReduction,
                                meterValues.gainReductionDb[0]);
            updateSmoothedValue(m_rightGainReduction,
                                meterValues.gainReductionDb[1]);
        }
        if (receivedMeterValues) {
            m_lastMeterValueTime.restart();
            m_meterDecayStarted = false;
        } else if (!m_meterDecayStarted
                   && m_lastMeterValueTime.elapsed() >= analysisIdleTimeout) {
            m_leftBandLevel.setTargetValue(meterFloorDb);
            m_rightBandLevel.setTargetValue(meterFloorDb);
            m_leftGainReduction.setTargetValue(0.0f);
            m_rightGainReduction.setTargetValue(0.0f);
            m_meterDecayStarted = true;
        }
        m_leftBandLevel.nextValue();
        m_rightBandLevel.nextValue();
        m_leftGainReduction.nextValue();
        m_rightGainReduction.nextValue();
        Q_EMIT levelsChanged();

        DeEsserProcessor::SpectrumFrame spectrumFrame;
        bool receivedSpectrumFrame = false;
        while (m_processor->takeSpectrumFrame(spectrumFrame)) {
            receivedSpectrumFrame = true;
            for (int index = 0; index < deEsserSpectrumBinCount; ++index) {
                auto &smoothed = m_smoothedSpectrum.at(static_cast<std::size_t>(index));
                updateSmoothedValue(
                    smoothed,
                    spectrumFrame.levelsDb.at(static_cast<std::size_t>(index)));
            }
        }
        if (receivedSpectrumFrame) {
            m_lastSpectrumFrameTime.restart();
            m_spectrumDecayStarted = false;
        } else if (!m_spectrumDecayStarted
                   && m_lastSpectrumFrameTime.elapsed() >= analysisIdleTimeout) {
            for (auto &value : m_smoothedSpectrum) {
                value.setTargetValue(meterFloorDb);
            }
            m_spectrumDecayStarted = true;
        }
        for (int index = 0; index < deEsserSpectrumBinCount; ++index) {
            m_spectrumCurve.at(static_cast<std::size_t>(index)) =
                m_smoothedSpectrum.at(static_cast<std::size_t>(index)).nextValue();
        }
        Q_EMIT spectrumCurveChanged();

        const qint64 elapsed = m_analysisTickTime.restart();
        m_analysisTimer->start(std::max(
            0, analysisRefreshInterval - static_cast<int>(elapsed)));
    }

    void DeEsserEffectsUnit::resetLevelDisplays() {
        m_leftBandLevel.setCurrentAndTargetValue(meterFloorDb);
        m_rightBandLevel.setCurrentAndTargetValue(meterFloorDb);
        m_leftGainReduction.setCurrentAndTargetValue(0.0f);
        m_rightGainReduction.setCurrentAndTargetValue(0.0f);
        Q_EMIT levelsChanged();
    }

    void DeEsserEffectsUnit::resetSpectrumDisplay() {
        for (auto &value : m_smoothedSpectrum) {
            value.setCurrentAndTargetValue(meterFloorDb);
        }
        m_spectrumCurve.fill(meterFloorDb);
        Q_EMIT spectrumCurveChanged();
    }

    DeEsserEffectsUnitClass::DeEsserEffectsUnitClass(QObject *parent)
        : EffectsUnitClass(tr("De-Esser"), parent),
          m_editorComponent(new QQmlComponent(Core::RuntimeInterface::qmlEngine(),
                                              QStringLiteral("DiffScope.DeEsserEffectsUnit"),
                                              QStringLiteral("DeEsserEditor"), this)) {
        if (m_editorComponent->isError()) {
            qFatal() << m_editorComponent->errorString();
        }
    }

    DeEsserEffectsUnitClass::~DeEsserEffectsUnitClass() = default;

    Audio::EffectsUnit *DeEsserEffectsUnitClass::create(QObject *parent) const {
        return new DeEsserEffectsUnit(m_editorComponent, parent);
    }

}

#include "moc_DeEsserEffectsUnit.cpp"
