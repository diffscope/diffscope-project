// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "CompressorEffectsUnit.h"

#include <algorithm>
#include <cmath>
#include <memory>

#include <QJsonObject>
#include <QQmlComponent>
#include <QQuickItem>
#include <QTimer>
#include <QVariant>

#include <CoreApi/runtimeinterface.h>

#include <compressoreffectsunit/internal/CompressorParameters.h>
#include <compressoreffectsunit/internal/CompressorProcessor.h>

namespace CompressorEffectsUnit::Internal {

    namespace {

        constexpr int levelMeterRampLength = 128;
        constexpr int levelMeterRefreshInterval = 8;
        constexpr int levelMeterIdleTimeout = 250;

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

        void updateSmoothedValue(talcs::SmoothedFloat &smoothedValue, float value) {
            if (value < smoothedValue.currentValue()) {
                smoothedValue.setTargetValue(value);
            } else {
                smoothedValue.setCurrentAndTargetValue(value);
            }
        }

    }

    CompressorEffectsUnit::CompressorEffectsUnit(QQmlComponent *editorComponent, QObject *parent)
        : EffectsUnit(parent),
          m_inputLevel(meterFloorDb),
          m_leftOutputLevel(meterFloorDb),
          m_rightOutputLevel(meterFloorDb),
          m_leftGainReduction(0.0f),
          m_rightGainReduction(0.0f),
          m_committedThresholdDb(defaultThresholdDb),
          m_committedRatio(defaultRatio),
          m_committedAttackMilliseconds(defaultAttackMilliseconds),
          m_committedReleaseMilliseconds(defaultReleaseMilliseconds),
          m_thresholdDb(defaultThresholdDb),
          m_ratio(defaultRatio),
          m_attackMilliseconds(defaultAttackMilliseconds),
          m_releaseMilliseconds(defaultReleaseMilliseconds) {
        auto processor = std::make_unique<CompressorProcessor>();
        m_processor = processor.get();
        setProcessor(std::move(processor));
        updateProcessor();

        m_inputLevel.setRampLength(levelMeterRampLength);
        m_leftOutputLevel.setRampLength(levelMeterRampLength);
        m_rightOutputLevel.setRampLength(levelMeterRampLength);
        m_leftGainReduction.setRampLength(levelMeterRampLength);
        m_rightGainReduction.setRampLength(levelMeterRampLength);

        m_meterTimer = new QTimer(this);
        m_meterTimer->setSingleShot(true);
        connect(m_meterTimer, &QTimer::timeout, this, &CompressorEffectsUnit::tickMeters);

        auto object = editorComponent->createWithInitialProperties({
            {QStringLiteral("effectsUnit"), QVariant::fromValue(this)},
        }, editorComponent->creationContext());
        if (!object) {
            qFatal() << editorComponent->errorString();
        }
        auto editor = qobject_cast<QQuickItem *>(object);
        if (!editor) {
            delete object;
            qFatal("CompressorEditor must create a QQuickItem");
        }
        setEditor(editor);
        connect(editor, &QQuickItem::windowChanged, this,
                [this] { updateMeterTimer(); });
        connect(editor, &QQuickItem::visibleChanged, this,
                [this] { updateMeterTimer(); });
        updateMeterTimer();
    }

    CompressorEffectsUnit::~CompressorEffectsUnit() {
        stopMeterTimer();
    }

    double CompressorEffectsUnit::thresholdDb() const {
        return m_thresholdDb;
    }

    double CompressorEffectsUnit::ratio() const {
        return m_ratio;
    }

    double CompressorEffectsUnit::attackMilliseconds() const {
        return m_attackMilliseconds;
    }

    double CompressorEffectsUnit::releaseMilliseconds() const {
        return m_releaseMilliseconds;
    }

    double CompressorEffectsUnit::inputLevelDb() const {
        return m_inputLevel.currentValue();
    }

    double CompressorEffectsUnit::leftOutputLevelDb() const {
        return m_leftOutputLevel.currentValue();
    }

    double CompressorEffectsUnit::rightOutputLevelDb() const {
        return m_rightOutputLevel.currentValue();
    }

    double CompressorEffectsUnit::leftGainReductionDb() const {
        return m_leftGainReduction.currentValue();
    }

    double CompressorEffectsUnit::rightGainReductionDb() const {
        return m_rightGainReduction.currentValue();
    }

    QJsonValue CompressorEffectsUnit::getState() const {
        return QJsonObject{
            {QStringLiteral("thresholdDb"), m_committedThresholdDb},
            {QStringLiteral("ratio"), m_committedRatio},
            {QStringLiteral("attackMilliseconds"), m_committedAttackMilliseconds},
            {QStringLiteral("releaseMilliseconds"), m_committedReleaseMilliseconds},
        };
    }

    void CompressorEffectsUnit::setState(const QJsonValue &state) {
        const auto object = state.isObject() ? state.toObject() : QJsonObject{};
        const double threshold = normalizedValue(
            object.value(QStringLiteral("thresholdDb")), defaultThresholdDb,
            minimumThresholdDb, maximumThresholdDb);
        const double newRatio = normalizedValue(
            object.value(QStringLiteral("ratio")), defaultRatio,
            minimumRatio, maximumRatio);
        const double attack = normalizedValue(
            object.value(QStringLiteral("attackMilliseconds")), defaultAttackMilliseconds,
            minimumAttackMilliseconds, maximumAttackMilliseconds);
        const double release = normalizedValue(
            object.value(QStringLiteral("releaseMilliseconds")), defaultReleaseMilliseconds,
            minimumReleaseMilliseconds, maximumReleaseMilliseconds);

        const bool stateChanged = !valuesEqual(m_committedThresholdDb, threshold)
            || !valuesEqual(m_committedRatio, newRatio)
            || !valuesEqual(m_committedAttackMilliseconds, attack)
            || !valuesEqual(m_committedReleaseMilliseconds, release);
        const bool thresholdChanged = !valuesEqual(m_thresholdDb, threshold);
        const bool ratioValueChanged = !valuesEqual(m_ratio, newRatio);
        const bool attackChanged = !valuesEqual(m_attackMilliseconds, attack);
        const bool releaseChanged = !valuesEqual(m_releaseMilliseconds, release);

        m_committedThresholdDb = threshold;
        m_committedRatio = newRatio;
        m_committedAttackMilliseconds = attack;
        m_committedReleaseMilliseconds = release;
        m_thresholdDb = threshold;
        m_ratio = newRatio;
        m_attackMilliseconds = attack;
        m_releaseMilliseconds = release;
        updateProcessor();

        if (thresholdChanged) {
            Q_EMIT thresholdDbChanged();
        }
        if (ratioValueChanged) {
            Q_EMIT ratioChanged();
        }
        if (attackChanged) {
            Q_EMIT attackMillisecondsChanged();
        }
        if (releaseChanged) {
            Q_EMIT releaseMillisecondsChanged();
        }
        if (stateChanged) {
            Q_EMIT updated();
        }
    }

    void CompressorEffectsUnit::refresh() {
        m_processor->refresh();
        m_processor->discardMeterValues();
        resetMeterDisplays();
        m_meterDecayStarted = false;
        if (m_meterActive) {
            m_lastMeterValueTime.restart();
        }
    }

    void CompressorEffectsUnit::previewThresholdDb(double value) {
        if (previewValue(m_thresholdDb, value, minimumThresholdDb, maximumThresholdDb)) {
            Q_EMIT thresholdDbChanged();
        }
    }

    void CompressorEffectsUnit::previewRatio(double value) {
        if (previewValue(m_ratio, value, minimumRatio, maximumRatio)) {
            Q_EMIT ratioChanged();
        }
    }

    void CompressorEffectsUnit::previewAttackMilliseconds(double value) {
        if (previewValue(m_attackMilliseconds, value,
                         minimumAttackMilliseconds, maximumAttackMilliseconds)) {
            Q_EMIT attackMillisecondsChanged();
        }
    }

    void CompressorEffectsUnit::previewReleaseMilliseconds(double value) {
        if (previewValue(m_releaseMilliseconds, value,
                         minimumReleaseMilliseconds, maximumReleaseMilliseconds)) {
            Q_EMIT releaseMillisecondsChanged();
        }
    }

    void CompressorEffectsUnit::commitPreview() {
        if (valuesEqual(m_committedThresholdDb, m_thresholdDb)
            && valuesEqual(m_committedRatio, m_ratio)
            && valuesEqual(m_committedAttackMilliseconds, m_attackMilliseconds)
            && valuesEqual(m_committedReleaseMilliseconds, m_releaseMilliseconds)) {
            return;
        }
        m_committedThresholdDb = m_thresholdDb;
        m_committedRatio = m_ratio;
        m_committedAttackMilliseconds = m_attackMilliseconds;
        m_committedReleaseMilliseconds = m_releaseMilliseconds;
        Q_EMIT updated();
    }

    void CompressorEffectsUnit::setThresholdDb(double value) {
        previewThresholdDb(value);
        commitPreview();
    }

    void CompressorEffectsUnit::setRatio(double value) {
        previewRatio(value);
        commitPreview();
    }

    void CompressorEffectsUnit::setAttackMilliseconds(double value) {
        previewAttackMilliseconds(value);
        commitPreview();
    }

    void CompressorEffectsUnit::setReleaseMilliseconds(double value) {
        previewReleaseMilliseconds(value);
        commitPreview();
    }

    bool CompressorEffectsUnit::previewValue(double &member, double value,
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

    void CompressorEffectsUnit::updateProcessor() {
        m_processor->setParameters(m_thresholdDb, m_ratio,
                                   m_attackMilliseconds, m_releaseMilliseconds);
    }

    void CompressorEffectsUnit::updateMeterTimer() {
        const bool shouldRun = editor() && editor()->window() && editor()->isVisible();
        if (shouldRun) {
            startMeterTimer();
        } else {
            stopMeterTimer();
        }
    }

    void CompressorEffectsUnit::startMeterTimer() {
        if (m_meterActive) {
            return;
        }
        m_processor->discardMeterValues();
        m_processor->setMeterEnabled(true);
        resetMeterDisplays();
        m_meterActive = true;
        m_meterDecayStarted = false;
        m_meterTickTime.start();
        m_lastMeterValueTime.start();
        tickMeters();
    }

    void CompressorEffectsUnit::stopMeterTimer() {
        m_processor->setMeterEnabled(false);
        if (!m_meterActive) {
            return;
        }
        m_meterActive = false;
        m_meterTimer->stop();
        m_processor->discardMeterValues();
        resetMeterDisplays();
    }

    void CompressorEffectsUnit::tickMeters() {
        if (!m_meterActive) {
            return;
        }
        if (!editor() || !editor()->window() || !editor()->isVisible()) {
            stopMeterTimer();
            return;
        }

        CompressorProcessor::MeterValues values;
        bool receivedMeterValues = false;
        while (m_processor->takeMeterValues(values)) {
            receivedMeterValues = true;
            updateSmoothedValue(m_inputLevel, values.inputLevelDb);
            updateSmoothedValue(m_leftOutputLevel, values.outputLevelDb[0]);
            updateSmoothedValue(m_rightOutputLevel, values.outputLevelDb[1]);
            updateSmoothedValue(m_leftGainReduction, values.gainReductionDb[0]);
            updateSmoothedValue(m_rightGainReduction, values.gainReductionDb[1]);
        }
        if (receivedMeterValues) {
            m_lastMeterValueTime.restart();
            m_meterDecayStarted = false;
        } else if (!m_meterDecayStarted
                   && m_lastMeterValueTime.elapsed() >= levelMeterIdleTimeout) {
            m_inputLevel.setTargetValue(meterFloorDb);
            m_leftOutputLevel.setTargetValue(meterFloorDb);
            m_rightOutputLevel.setTargetValue(meterFloorDb);
            m_leftGainReduction.setTargetValue(0.0f);
            m_rightGainReduction.setTargetValue(0.0f);
            m_meterDecayStarted = true;
        }

        m_inputLevel.nextValue();
        m_leftOutputLevel.nextValue();
        m_rightOutputLevel.nextValue();
        m_leftGainReduction.nextValue();
        m_rightGainReduction.nextValue();
        Q_EMIT levelsChanged();

        const qint64 elapsed = m_meterTickTime.restart();
        m_meterTimer->start(std::max(0, levelMeterRefreshInterval - static_cast<int>(elapsed)));
    }

    void CompressorEffectsUnit::resetMeterDisplays() {
        m_inputLevel.setCurrentAndTargetValue(meterFloorDb);
        m_leftOutputLevel.setCurrentAndTargetValue(meterFloorDb);
        m_rightOutputLevel.setCurrentAndTargetValue(meterFloorDb);
        m_leftGainReduction.setCurrentAndTargetValue(0.0f);
        m_rightGainReduction.setCurrentAndTargetValue(0.0f);
        Q_EMIT levelsChanged();
    }

    CompressorEffectsUnitClass::CompressorEffectsUnitClass(QObject *parent)
        : EffectsUnitClass(tr("Compressor"), parent),
          m_editorComponent(new QQmlComponent(Core::RuntimeInterface::qmlEngine(),
                                              QStringLiteral("DiffScope.CompressorEffectsUnit"),
                                              QStringLiteral("CompressorEditor"), this)) {
        if (m_editorComponent->isError()) {
            qFatal() << m_editorComponent->errorString();
        }
    }

    CompressorEffectsUnitClass::~CompressorEffectsUnitClass() = default;

    Audio::EffectsUnit *CompressorEffectsUnitClass::create(QObject *parent) const {
        return new CompressorEffectsUnit(m_editorComponent, parent);
    }

}

#include "moc_CompressorEffectsUnit.cpp"
