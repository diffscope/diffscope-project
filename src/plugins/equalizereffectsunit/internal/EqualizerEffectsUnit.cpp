// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "EqualizerEffectsUnit.h"

#include <algorithm>
#include <cmath>
#include <memory>

#include <QAbstractItemModel>
#include <QJsonArray>
#include <QJsonObject>
#include <QQmlComponent>
#include <QQuickItem>
#include <QTimer>
#include <QVariant>

#include <CoreApi/runtimeinterface.h>

#include <equalizereffectsunit/internal/EqualizerBandModel.h>
#include <equalizereffectsunit/internal/EqualizerProcessor.h>

#include <signalsmith-dsp/filters.h>

namespace EqualizerEffectsUnit::Internal {

    namespace {

        constexpr int spectrumRefreshInterval = 16;
        constexpr int spectrumIdleTimeout = 250;
        constexpr int spectrumRampLength = 64;
        constexpr float spectrumFloorDb = -96.0f;

        bool valuesEqual(double left, double right) {
            return qFuzzyIsNull(left - right);
        }

        bool bandsEqual(const EqualizerBand &left, const EqualizerBand &right) {
            return left.type == right.type
                && valuesEqual(left.frequencyHz, right.frequencyHz)
                && valuesEqual(left.gainDb, right.gainDb)
                && valuesEqual(left.q, right.q)
                && left.enabled == right.enabled
                && left.solo == right.solo;
        }

        bool bandListsEqual(const EqualizerBandList &left,
                            const EqualizerBandList &right) {
            if (left.size() != right.size()) {
                return false;
            }
            for (int index = 0; index < left.size(); ++index) {
                if (!bandsEqual(left.at(index), right.at(index))) {
                    return false;
                }
            }
            return true;
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

        EqualizerBandType bandTypeFromString(const QString &value) {
            if (value == QStringLiteral("lowShelf")) {
                return EqualizerBandType::LowShelf;
            }
            if (value == QStringLiteral("highShelf")) {
                return EqualizerBandType::HighShelf;
            }
            return EqualizerBandType::Bell;
        }

        QString stringFromBandType(EqualizerBandType type) {
            switch (type) {
                case EqualizerBandType::LowShelf:
                    return QStringLiteral("lowShelf");
                case EqualizerBandType::HighShelf:
                    return QStringLiteral("highShelf");
                case EqualizerBandType::Bell:
                default:
                    return QStringLiteral("bell");
            }
        }

        EqualizerBandList normalizedBands(const QJsonValue &state) {
            if (!state.isObject()) {
                return defaultEqualizerBands();
            }
            const auto bandsValue = state.toObject().value(QStringLiteral("bands"));
            if (!bandsValue.isArray()) {
                return defaultEqualizerBands();
            }

            EqualizerBandList bands;
            const auto array = bandsValue.toArray();
            const int count = std::min(static_cast<int>(array.size()),
                                       maximumBandCount);
            bands.reserve(count);
            for (int index = 0; index < count; ++index) {
                const auto object = array.at(index).isObject()
                    ? array.at(index).toObject()
                    : QJsonObject{};
                EqualizerBand band;
                band.type = bandTypeFromString(
                    object.value(QStringLiteral("type")).toString());
                band.frequencyHz = normalizedValue(
                    object.value(QStringLiteral("frequencyHz")), 1000.0,
                    minimumFrequencyHz, maximumFrequencyHz);
                band.gainDb = normalizedValue(
                    object.value(QStringLiteral("gainDb")), 0.0,
                    minimumGainDb, maximumGainDb);
                band.q = normalizedValue(object.value(QStringLiteral("q")), defaultQ,
                                         minimumQ, maximumQ);
                band.enabled = normalizedBoolean(
                    object.value(QStringLiteral("enabled")), true);
                band.solo = normalizedBoolean(
                    object.value(QStringLiteral("solo")), false);
                bands.append(band);
            }
            return bands;
        }

        void configureResponseFilter(signalsmith::filters::BiquadStatic<double> &filter,
                                     const EqualizerBand &band, double sampleRate) {
            const double scaledFrequency = std::clamp(
                band.frequencyHz / sampleRate, 1.0e-6, 0.499);
            using signalsmith::filters::BiquadDesign;
            switch (band.type) {
                case EqualizerBandType::LowShelf:
                    filter.lowShelfDbQ(scaledFrequency, band.gainDb, band.q,
                                       BiquadDesign::oneSided);
                    break;
                case EqualizerBandType::HighShelf:
                    filter.highShelfDbQ(scaledFrequency, band.gainDb, band.q,
                                        BiquadDesign::oneSided);
                    break;
                case EqualizerBandType::Bell:
                default:
                    filter.peakDbQ(scaledFrequency, band.gainDb, band.q,
                                   BiquadDesign::oneSided);
                    break;
            }
        }

    }

    EqualizerEffectsUnit::EqualizerEffectsUnit(QQmlComponent *editorComponent,
                                               QObject *parent)
        : EffectsUnit(parent),
          m_bandModel(new EqualizerBandModel(this)),
          m_committedBands(defaultEqualizerBands()) {
        m_bandModel->setBands(m_committedBands);

        auto processor = std::make_unique<EqualizerProcessor>();
        m_processor = processor.get();
        setProcessor(std::move(processor));
        updateProcessor();

        m_spectrumCurve.fill(spectrumFloorDb);
        for (auto &value : m_smoothedSpectrum) {
            value.setCurrentAndTargetValue(spectrumFloorDb);
            value.setRampLength(spectrumRampLength);
        }
        rebuildResponseCurve();

        m_spectrumTimer = new QTimer(this);
        m_spectrumTimer->setSingleShot(true);
        connect(m_spectrumTimer, &QTimer::timeout,
                this, &EqualizerEffectsUnit::tickSpectrum);

        auto object = editorComponent->createWithInitialProperties({
            {QStringLiteral("effectsUnit"), QVariant::fromValue(this)},
        }, editorComponent->creationContext());
        if (!object) {
            qFatal() << editorComponent->errorString();
        }
        auto editor = qobject_cast<QQuickItem *>(object);
        if (!editor) {
            delete object;
            qFatal("EqualizerEditor must create a QQuickItem");
        }
        setEditor(editor);
        connect(editor, &QQuickItem::windowChanged, this,
                [this] { updateSpectrumTimer(); });
        connect(editor, &QQuickItem::visibleChanged, this,
                [this] { updateSpectrumTimer(); });
        updateSpectrumTimer();
    }

    EqualizerEffectsUnit::~EqualizerEffectsUnit() {
        stopSpectrumTimer();
    }

    QAbstractItemModel *EqualizerEffectsUnit::bands() const {
        return m_bandModel;
    }

    int EqualizerEffectsUnit::bandCount() const {
        return m_bandModel->rowCount();
    }

    bool EqualizerEffectsUnit::canAddBand() const {
        return bandCount() < maximumBandCount;
    }

    int EqualizerEffectsUnit::currentIndex() const {
        return m_currentIndex;
    }

    bool EqualizerEffectsUnit::hasCurrentBand() const {
        return currentBand() != nullptr;
    }

    double EqualizerEffectsUnit::currentFrequencyHz() const {
        const auto band = currentBand();
        return band ? band->frequencyHz : 1000.0;
    }

    double EqualizerEffectsUnit::currentGainDb() const {
        const auto band = currentBand();
        return band ? band->gainDb : 0.0;
    }

    double EqualizerEffectsUnit::currentQ() const {
        const auto band = currentBand();
        return band ? band->q : defaultQ;
    }

    EqualizerEffectsUnit::BandType EqualizerEffectsUnit::currentType() const {
        const auto band = currentBand();
        return static_cast<BandType>(band ? static_cast<int>(band->type)
                                          : static_cast<int>(EqualizerBandType::Bell));
    }

    bool EqualizerEffectsUnit::currentEnabled() const {
        const auto band = currentBand();
        return band && band->enabled;
    }

    bool EqualizerEffectsUnit::currentSolo() const {
        const auto band = currentBand();
        return band && band->solo;
    }

    QJsonValue EqualizerEffectsUnit::getState() const {
        QJsonArray bands;
        for (const auto &band : m_committedBands) {
            bands.append(QJsonObject{
                {QStringLiteral("type"), stringFromBandType(band.type)},
                {QStringLiteral("frequencyHz"), band.frequencyHz},
                {QStringLiteral("gainDb"), band.gainDb},
                {QStringLiteral("q"), band.q},
                {QStringLiteral("enabled"), band.enabled},
                {QStringLiteral("solo"), band.solo},
            });
        }
        return QJsonObject{{QStringLiteral("bands"), bands}};
    }

    void EqualizerEffectsUnit::setState(const QJsonValue &state) {
        const auto newBands = normalizedBands(state);
        const bool stateChanged = !bandListsEqual(m_committedBands, newBands);
        const bool currentValuesChanged = !bandListsEqual(m_bandModel->bands(), newBands);
        if (!stateChanged && !currentValuesChanged) {
            return;
        }
        const int oldCount = bandCount();
        const int oldCurrentIndex = m_currentIndex;

        m_committedBands = newBands;
        m_bandModel->setBands(newBands);
        if (newBands.isEmpty()) {
            m_currentIndex = -1;
        } else if (oldCurrentIndex >= 0 && oldCurrentIndex < newBands.size()) {
            m_currentIndex = oldCurrentIndex;
        } else {
            m_currentIndex = medianFrequencyBandIndex();
        }
        updateProcessor();
        rebuildResponseCurve();

        if (oldCount != bandCount()) {
            Q_EMIT bandCountChanged();
        }
        if (oldCurrentIndex != m_currentIndex) {
            Q_EMIT currentIndexChanged();
        }
        if (currentValuesChanged || oldCurrentIndex != m_currentIndex) {
            Q_EMIT currentBandChanged();
        }
        if (stateChanged) {
            Q_EMIT updated();
        }
    }

    void EqualizerEffectsUnit::refresh() {
        m_processor->refresh();
        m_processor->discardSpectrumFrames();
        resetSpectrumDisplay();
        m_spectrumDecayStarted = false;
        if (m_spectrumActive) {
            m_lastSpectrumFrameTime.restart();
        }
    }

    void EqualizerEffectsUnit::selectBand(int index) {
        if (index < 0 || index >= bandCount()) {
            return;
        }
        setCurrentIndex(index);
    }

    void EqualizerEffectsUnit::selectPreviousBand() {
        const auto orderedIndices = frequencyOrderedIndices();
        if (orderedIndices.isEmpty()) {
            return;
        }
        const int position = orderedIndices.indexOf(m_currentIndex);
        setCurrentIndex(position < 0
                            ? orderedIndices.first()
                            : orderedIndices.at((position - 1 + orderedIndices.size())
                                                % orderedIndices.size()));
    }

    void EqualizerEffectsUnit::selectNextBand() {
        const auto orderedIndices = frequencyOrderedIndices();
        if (orderedIndices.isEmpty()) {
            return;
        }
        const int position = orderedIndices.indexOf(m_currentIndex);
        setCurrentIndex(position < 0
                            ? orderedIndices.first()
                            : orderedIndices.at((position + 1) % orderedIndices.size()));
    }

    void EqualizerEffectsUnit::addBand() {
        addBandAt(largestFrequencyGapMidpoint(), 0.0);
    }

    void EqualizerEffectsUnit::addBandAt(double frequencyHz, double gainDb) {
        if (!canAddBand() || !std::isfinite(frequencyHz) || !std::isfinite(gainDb)) {
            return;
        }
        const EqualizerBand band{
            EqualizerBandType::Bell,
            std::clamp(frequencyHz, minimumFrequencyHz, maximumFrequencyHz),
            std::clamp(gainDb, minimumGainDb, maximumGainDb),
            defaultQ,
        };
        const int index = bandCount();
        m_bandModel->insertBand(index, band);
        Q_EMIT bandCountChanged();
        setCurrentIndex(index);
        updateProcessor();
        rebuildResponseCurve();
        commitPreview();
    }

    void EqualizerEffectsUnit::removeBand(int index) {
        if (index < 0 || index >= bandCount()) {
            return;
        }

        const int oldCurrentIndex = m_currentIndex;
        int newCurrentIndex = oldCurrentIndex;
        bool currentIdentityChanged = false;
        if (index == oldCurrentIndex) {
            currentIdentityChanged = true;
            if (bandCount() == 1) {
                newCurrentIndex = -1;
            } else {
                const auto orderedIndices = frequencyOrderedIndices();
                const int position = orderedIndices.indexOf(index);
                newCurrentIndex = orderedIndices.at((position + 1) % orderedIndices.size());
            }
        }

        m_bandModel->removeBand(index);
        if (newCurrentIndex > index) {
            --newCurrentIndex;
        }
        m_currentIndex = newCurrentIndex;
        Q_EMIT bandCountChanged();
        if (oldCurrentIndex != m_currentIndex) {
            Q_EMIT currentIndexChanged();
        }
        if (currentIdentityChanged) {
            Q_EMIT currentBandChanged();
        }
        updateProcessor();
        rebuildResponseCurve();
        commitPreview();
    }

    void EqualizerEffectsUnit::removeCurrentBand() {
        removeBand(m_currentIndex);
    }

    void EqualizerEffectsUnit::previewBandPosition(int index, double frequencyHz,
                                                   double gainDb) {
        const auto current = m_bandModel->bandAt(index);
        if (!current || !std::isfinite(frequencyHz) || !std::isfinite(gainDb)) {
            return;
        }
        auto band = *current;
        band.frequencyHz = std::clamp(frequencyHz, minimumFrequencyHz,
                                      maximumFrequencyHz);
        band.gainDb = std::clamp(gainDb, minimumGainDb, maximumGainDb);
        previewBand(index, band);
    }

    void EqualizerEffectsUnit::previewBandQ(int index, double value) {
        const auto current = m_bandModel->bandAt(index);
        if (!current || !std::isfinite(value)) {
            return;
        }
        auto band = *current;
        band.q = std::clamp(value, minimumQ, maximumQ);
        previewBand(index, band);
    }

    void EqualizerEffectsUnit::previewCurrentFrequencyHz(double value) {
        previewBandPosition(m_currentIndex, value, currentGainDb());
    }

    void EqualizerEffectsUnit::previewCurrentGainDb(double value) {
        previewBandPosition(m_currentIndex, currentFrequencyHz(), value);
    }

    void EqualizerEffectsUnit::previewCurrentQ(double value) {
        previewBandQ(m_currentIndex, value);
    }

    void EqualizerEffectsUnit::commitPreview() {
        if (bandListsEqual(m_committedBands, m_bandModel->bands())) {
            return;
        }
        m_committedBands = m_bandModel->bands();
        Q_EMIT updated();
    }

    void EqualizerEffectsUnit::setCurrentFrequencyHz(double value) {
        previewCurrentFrequencyHz(value);
        commitPreview();
    }

    void EqualizerEffectsUnit::setCurrentGainDb(double value) {
        previewCurrentGainDb(value);
        commitPreview();
    }

    void EqualizerEffectsUnit::setCurrentQ(double value) {
        previewCurrentQ(value);
        commitPreview();
    }

    void EqualizerEffectsUnit::setCurrentType(BandType type) {
        const auto current = currentBand();
        if (!current || type < Bell || type > HighShelf) {
            return;
        }
        auto band = *current;
        band.type = static_cast<EqualizerBandType>(static_cast<int>(type));
        if (previewBand(m_currentIndex, band)) {
            commitPreview();
        }
    }

    void EqualizerEffectsUnit::setCurrentEnabled(bool enabled) {
        const auto current = currentBand();
        if (!current) {
            return;
        }
        auto band = *current;
        band.enabled = enabled;
        if (previewBand(m_currentIndex, band)) {
            commitPreview();
        }
    }

    void EqualizerEffectsUnit::setCurrentSolo(bool solo) {
        const auto current = currentBand();
        if (!current) {
            return;
        }
        auto band = *current;
        band.solo = solo;
        if (previewBand(m_currentIndex, band)) {
            commitPreview();
        }
    }

    const std::array<float, equalizerResponsePointCount> &
        EqualizerEffectsUnit::responseCurveDb() const {
        return m_responseCurve;
    }

    const std::array<float, equalizerSpectrumBinCount> &
        EqualizerEffectsUnit::spectrumCurveDb() const {
        return m_spectrumCurve;
    }

    const EqualizerBand *EqualizerEffectsUnit::currentBand() const {
        return m_bandModel->bandAt(m_currentIndex);
    }

    void EqualizerEffectsUnit::setCurrentIndex(int index) {
        if (index == m_currentIndex || index < -1 || index >= bandCount()) {
            return;
        }
        m_currentIndex = index;
        Q_EMIT currentIndexChanged();
        Q_EMIT currentBandChanged();
    }

    QList<int> EqualizerEffectsUnit::frequencyOrderedIndices() const {
        QList<int> indices;
        indices.reserve(bandCount());
        for (int index = 0; index < bandCount(); ++index) {
            indices.append(index);
        }
        std::ranges::sort(indices, [this](int left, int right) {
            const auto leftBand = m_bandModel->bandAt(left);
            const auto rightBand = m_bandModel->bandAt(right);
            if (leftBand->frequencyHz != rightBand->frequencyHz) {
                return leftBand->frequencyHz < rightBand->frequencyHz;
            }
            return left < right;
        });
        return indices;
    }

    int EqualizerEffectsUnit::medianFrequencyBandIndex() const {
        const auto indices = frequencyOrderedIndices();
        return indices.isEmpty() ? -1 : indices.at(indices.size() / 2);
    }

    double EqualizerEffectsUnit::largestFrequencyGapMidpoint() const {
        QList<double> frequencies;
        frequencies.reserve(bandCount() + 2);
        frequencies.append(minimumFrequencyHz);
        for (const auto &band : m_bandModel->bands()) {
            frequencies.append(band.frequencyHz);
        }
        frequencies.append(maximumFrequencyHz);
        std::ranges::sort(frequencies);

        double bestLower = frequencies.first();
        double bestUpper = frequencies.at(1);
        double bestGap = std::log(bestUpper / bestLower);
        for (int index = 1; index + 1 < frequencies.size(); ++index) {
            const double lower = frequencies.at(index);
            const double upper = frequencies.at(index + 1);
            const double gap = std::log(upper / lower);
            if (gap > bestGap + 1.0e-12) {
                bestLower = lower;
                bestUpper = upper;
                bestGap = gap;
            }
        }
        return std::sqrt(bestLower * bestUpper);
    }

    bool EqualizerEffectsUnit::hasSoloBands() const {
        const auto &bands = m_bandModel->bands();
        return std::ranges::any_of(bands, [](const auto &band) {
            return band.solo;
        });
    }

    EqualizerBandList EqualizerEffectsUnit::processingBands(bool soloMode) const {
        const auto &bands = m_bandModel->bands();
        EqualizerBandList result;
        result.reserve(bands.size());
        for (const auto &band : bands) {
            if (soloMode ? band.solo : band.enabled) {
                result.append(band);
            }
        }
        return result;
    }

    EqualizerBandList EqualizerEffectsUnit::responseBands(bool soloMode) const {
        const auto &bands = m_bandModel->bands();
        EqualizerBandList result;
        result.reserve(bands.size());
        for (const auto &band : bands) {
            if (band.enabled && (!soloMode || band.solo)) {
                result.append(band);
            }
        }
        return result;
    }

    bool EqualizerEffectsUnit::previewBand(int index, const EqualizerBand &band) {
        if (!m_bandModel->updateBand(index, band)) {
            return false;
        }
        updateProcessor();
        rebuildResponseCurve();
        if (index == m_currentIndex) {
            Q_EMIT currentBandChanged();
        }
        return true;
    }

    void EqualizerEffectsUnit::updateProcessor() {
        const bool soloMode = hasSoloBands();
        m_processor->setBands(processingBands(soloMode), soloMode);
    }

    void EqualizerEffectsUnit::rebuildResponseCurve() {
        m_responseSampleRate = std::max(1.0, m_processor->currentSampleRate());
        std::array<signalsmith::filters::BiquadStatic<double>, maximumBandCount> filters;
        const bool soloMode = hasSoloBands();
        const auto bands = responseBands(soloMode);
        const int count = bands.size();
        for (int index = 0; index < count; ++index) {
            configureResponseFilter(filters.at(static_cast<std::size_t>(index)),
                                    bands.at(index),
                                    m_responseSampleRate);
        }

        for (int index = 0; index < equalizerResponsePointCount; ++index) {
            const double normalizedPosition = static_cast<double>(index)
                / static_cast<double>(equalizerResponsePointCount - 1);
            const double frequency = minimumFrequencyHz
                * std::pow(maximumFrequencyHz / minimumFrequencyHz,
                           normalizedPosition);
            const double scaledFrequency = std::clamp(
                frequency / m_responseSampleRate, 1.0e-6, 0.499);
            double responseDb = 0.0;
            for (int bandIndex = 0; bandIndex < count; ++bandIndex) {
                responseDb += filters.at(static_cast<std::size_t>(bandIndex))
                                  .responseDb(scaledFrequency);
            }
            m_responseCurve.at(static_cast<std::size_t>(index)) =
                static_cast<float>(std::clamp(responseDb, minimumGainDb,
                                              maximumGainDb));
        }
        Q_EMIT responseCurveChanged();
    }

    void EqualizerEffectsUnit::updateSpectrumTimer() {
        const bool shouldRun = editor() && editor()->window() && editor()->isVisible();
        if (shouldRun) {
            startSpectrumTimer();
        } else {
            stopSpectrumTimer();
        }
    }

    void EqualizerEffectsUnit::startSpectrumTimer() {
        if (m_spectrumActive) {
            return;
        }
        m_processor->discardSpectrumFrames();
        m_processor->setSpectrumEnabled(true);
        resetSpectrumDisplay();
        m_spectrumActive = true;
        m_spectrumDecayStarted = false;
        m_spectrumTickTime.start();
        m_lastSpectrumFrameTime.start();
        tickSpectrum();
    }

    void EqualizerEffectsUnit::stopSpectrumTimer() {
        if (!m_spectrumActive) {
            m_processor->setSpectrumEnabled(false);
            return;
        }
        m_spectrumActive = false;
        m_spectrumTimer->stop();
        m_processor->setSpectrumEnabled(false);
        m_processor->discardSpectrumFrames();
        resetSpectrumDisplay();
    }

    void EqualizerEffectsUnit::tickSpectrum() {
        if (!m_spectrumActive) {
            return;
        }
        if (!editor() || !editor()->window() || !editor()->isVisible()) {
            stopSpectrumTimer();
            return;
        }

        const double sampleRate = m_processor->currentSampleRate();
        if (!valuesEqual(sampleRate, m_responseSampleRate)) {
            rebuildResponseCurve();
        }

        EqualizerProcessor::SpectrumFrame frame;
        bool receivedFrame = false;
        while (m_processor->takeSpectrumFrame(frame)) {
            receivedFrame = true;
            for (int index = 0; index < equalizerSpectrumBinCount; ++index) {
                auto &smoothed = m_smoothedSpectrum.at(static_cast<std::size_t>(index));
                const float value = frame.levelsDb.at(static_cast<std::size_t>(index));
                if (value < smoothed.currentValue()) {
                    smoothed.setTargetValue(value);
                } else {
                    smoothed.setCurrentAndTargetValue(value);
                }
            }
        }
        if (receivedFrame) {
            m_lastSpectrumFrameTime.restart();
            m_spectrumDecayStarted = false;
        } else if (!m_spectrumDecayStarted
                   && m_lastSpectrumFrameTime.elapsed() >= spectrumIdleTimeout) {
            for (auto &value : m_smoothedSpectrum) {
                value.setTargetValue(spectrumFloorDb);
            }
            m_spectrumDecayStarted = true;
        }

        for (int index = 0; index < equalizerSpectrumBinCount; ++index) {
            m_spectrumCurve.at(static_cast<std::size_t>(index)) =
                m_smoothedSpectrum.at(static_cast<std::size_t>(index)).nextValue();
        }
        Q_EMIT spectrumCurveChanged();

        const qint64 elapsed = m_spectrumTickTime.restart();
        m_spectrumTimer->start(std::max(
            0, spectrumRefreshInterval - static_cast<int>(elapsed)));
    }

    void EqualizerEffectsUnit::resetSpectrumDisplay() {
        for (auto &value : m_smoothedSpectrum) {
            value.setCurrentAndTargetValue(spectrumFloorDb);
        }
        m_spectrumCurve.fill(spectrumFloorDb);
        Q_EMIT spectrumCurveChanged();
    }

    EqualizerEffectsUnitClass::EqualizerEffectsUnitClass(QObject *parent)
        : EffectsUnitClass(tr("Equalizer"), parent),
          m_editorComponent(new QQmlComponent(Core::RuntimeInterface::qmlEngine(),
                                              QStringLiteral("DiffScope.EqualizerEffectsUnit"),
                                              QStringLiteral("EqualizerEditor"), this)) {
        if (m_editorComponent->isError()) {
            qFatal() << m_editorComponent->errorString();
        }
    }

    EqualizerEffectsUnitClass::~EqualizerEffectsUnitClass() = default;

    Audio::EffectsUnit *EqualizerEffectsUnitClass::create(QObject *parent) const {
        return new EqualizerEffectsUnit(m_editorComponent, parent);
    }

}

#include "moc_EqualizerEffectsUnit.cpp"
