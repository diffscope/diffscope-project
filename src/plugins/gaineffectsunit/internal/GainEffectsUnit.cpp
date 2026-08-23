// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "GainEffectsUnit.h"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <memory>

#include <QJsonObject>
#include <QQmlComponent>
#include <QQuickItem>
#include <QVariant>

#include <CoreApi/runtimeinterface.h>

#include <TalcsCore/AudioSource.h>

namespace GainEffectsUnit::Internal {

    namespace {

        constexpr double minimumGainDb = -96.0;
        constexpr double maximumGainDb = 12.0;

        double normalizedGain(const QJsonValue &value) {
            if (!value.isDouble() || !std::isfinite(value.toDouble())) {
                return 0.0;
            }
            return std::clamp(value.toDouble(), minimumGainDb, maximumGainDb);
        }

        bool gainsEqual(double left, double right) {
            return qFuzzyIsNull(left - right);
        }

        float gainFromDecibels(double decibels) {
            return decibels <= minimumGainDb
                ? 0.0f
                : static_cast<float>(std::pow(10.0, decibels * 0.05));
        }

    }

    class GainProcessor final : public talcs::AudioSource {
    public:
        void setGains(double leftGainDb, double rightGainDb) {
            m_leftGain.store(gainFromDecibels(leftGainDb), std::memory_order_relaxed);
            m_rightGain.store(gainFromDecibels(rightGainDb), std::memory_order_relaxed);
        }

    protected:
        qint64 processReading(const talcs::AudioSourceReadData &readData) override {
            const int channelCount = std::min(2, readData.buffer->channelCount());
            for (int channel = 0; channel < channelCount; ++channel) {
                const float gain = channel == 0
                    ? m_leftGain.load(std::memory_order_relaxed)
                    : m_rightGain.load(std::memory_order_relaxed);
                for (qint64 position = readData.startPos;
                     position < readData.startPos + readData.length; ++position) {
                    readData.buffer->setSample(
                        channel, position,
                        readData.buffer->sample(channel, position) * gain);
                }
            }
            return readData.length;
        }

    private:
        std::atomic<float> m_leftGain{1.0f};
        std::atomic<float> m_rightGain{1.0f};
    };

    GainEffectsUnit::GainEffectsUnit(QQmlComponent *editorComponent, QObject *parent)
        : EffectsUnit(parent) {
        auto processor = std::make_unique<GainProcessor>();
        m_processor = processor.get();
        setProcessor(std::move(processor));
        updateProcessor();

        auto object = editorComponent->createWithInitialProperties({
            {QStringLiteral("effectsUnit"), QVariant::fromValue(this)},
        }, editorComponent->creationContext());
        if (!object) {
            qFatal() << editorComponent->errorString();
        }
        auto editor = qobject_cast<QQuickItem *>(object);
        if (!editor) {
            delete object;
            qFatal("GainEditor must create a QQuickItem");
        }
        setEditor(editor);
    }

    GainEffectsUnit::~GainEffectsUnit() = default;

    double GainEffectsUnit::leftGainDb() const {
        return m_leftGainDb;
    }

    double GainEffectsUnit::rightGainDb() const {
        return m_rightGainDb;
    }

    bool GainEffectsUnit::channelsLinked() const {
        return m_channelsLinked;
    }

    QJsonValue GainEffectsUnit::getState() const {
        return QJsonObject{
            {QStringLiteral("leftGainDb"), m_committedLeftGainDb},
            {QStringLiteral("rightGainDb"), m_committedRightGainDb},
            {QStringLiteral("channelsLinked"), m_channelsLinked},
        };
    }

    void GainEffectsUnit::setState(const QJsonValue &state) {
        const auto object = state.isObject() ? state.toObject() : QJsonObject{};
        const double left = normalizedGain(object.value(QStringLiteral("leftGainDb")));
        const double right = normalizedGain(object.value(QStringLiteral("rightGainDb")));
        const bool linked = object.value(QStringLiteral("channelsLinked")).isBool()
            ? object.value(QStringLiteral("channelsLinked")).toBool()
            : true;
        const bool stateChanged = !gainsEqual(m_committedLeftGainDb, left)
            || !gainsEqual(m_committedRightGainDb, right)
            || m_channelsLinked != linked;
        const bool leftChanged = !gainsEqual(m_leftGainDb, left);
        const bool rightChanged = !gainsEqual(m_rightGainDb, right);
        const bool linkedChanged = m_channelsLinked != linked;

        m_committedLeftGainDb = left;
        m_committedRightGainDb = right;
        m_leftGainDb = left;
        m_rightGainDb = right;
        m_channelsLinked = linked;
        updateProcessor();

        if (leftChanged) {
            Q_EMIT leftGainDbChanged();
        }
        if (rightChanged) {
            Q_EMIT rightGainDbChanged();
        }
        if (linkedChanged) {
            Q_EMIT channelsLinkedChanged();
        }
        if (stateChanged) {
            Q_EMIT updated();
        }
    }

    void GainEffectsUnit::previewLeftGainDb(double value) {
        previewGain(0, value);
    }

    void GainEffectsUnit::previewRightGainDb(double value) {
        previewGain(1, value);
    }

    void GainEffectsUnit::commitPreview() {
        if (gainsEqual(m_committedLeftGainDb, m_leftGainDb)
            && gainsEqual(m_committedRightGainDb, m_rightGainDb)) {
            return;
        }
        m_committedLeftGainDb = m_leftGainDb;
        m_committedRightGainDb = m_rightGainDb;
        Q_EMIT updated();
    }

    void GainEffectsUnit::setLeftGainDb(double value) {
        previewGain(0, value);
        commitPreview();
    }

    void GainEffectsUnit::setRightGainDb(double value) {
        previewGain(1, value);
        commitPreview();
    }

    void GainEffectsUnit::setChannelsLinked(bool linked) {
        if (m_channelsLinked == linked) {
            return;
        }
        m_committedLeftGainDb = m_leftGainDb;
        m_committedRightGainDb = m_rightGainDb;
        m_channelsLinked = linked;
        Q_EMIT channelsLinkedChanged();
        Q_EMIT updated();
    }

    void GainEffectsUnit::previewGain(int channel, double value) {
        value = std::clamp(value, minimumGainDb, maximumGainDb);
        double left = m_leftGainDb;
        double right = m_rightGainDb;
        if (m_channelsLinked) {
            const double current = channel == 0 ? left : right;
            const double minimumDelta = std::max(minimumGainDb - left, minimumGainDb - right);
            const double maximumDelta = std::min(maximumGainDb - left, maximumGainDb - right);
            const double delta = std::clamp(value - current, minimumDelta, maximumDelta);
            left += delta;
            right += delta;
        } else if (channel == 0) {
            left = value;
        } else {
            right = value;
        }
        const bool leftChanged = !gainsEqual(m_leftGainDb, left);
        const bool rightChanged = !gainsEqual(m_rightGainDb, right);
        if (!leftChanged && !rightChanged) {
            return;
        }
        m_leftGainDb = left;
        m_rightGainDb = right;
        updateProcessor();
        if (leftChanged) {
            Q_EMIT leftGainDbChanged();
        }
        if (rightChanged) {
            Q_EMIT rightGainDbChanged();
        }
    }

    void GainEffectsUnit::updateProcessor() {
        m_processor->setGains(m_leftGainDb, m_rightGainDb);
    }

    GainEffectsUnitClass::GainEffectsUnitClass(QObject *parent)
        : EffectsUnitClass(tr("Gain"), parent),
          m_editorComponent(new QQmlComponent(Core::RuntimeInterface::qmlEngine(),
                                              QStringLiteral("DiffScope.GainEffectsUnit"),
                                              QStringLiteral("GainEditor"), this)) {
        if (m_editorComponent->isError()) {
            qFatal() << m_editorComponent->errorString();
        }
    }

    GainEffectsUnitClass::~GainEffectsUnitClass() = default;

    EffectsUnitManager::EffectsUnit *GainEffectsUnitClass::create(QObject *parent) const {
        return new GainEffectsUnit(m_editorComponent, parent);
    }

}

#include "moc_GainEffectsUnit.cpp"
