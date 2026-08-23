// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "ChannelMapperEffectsUnit.h"

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

namespace ChannelMapperEffectsUnit::Internal {

    namespace {

        constexpr double minimumMixPercent = -100.0;
        constexpr double maximumMixPercent = 100.0;

        double normalizedMixPercent(const QJsonValue &value) {
            if (!value.isDouble() || !std::isfinite(value.toDouble())) {
                return 0.0;
            }
            return std::clamp(value.toDouble(), minimumMixPercent, maximumMixPercent);
        }

        bool mixValuesEqual(double left, double right) {
            return qFuzzyIsNull(left - right);
        }

        float mixCoefficient(double percent) {
            return static_cast<float>(percent * 0.01);
        }

    }

    class ChannelMapperProcessor final : public talcs::AudioSource {
    public:
        void setMixCoefficients(
            double leftLeftPercent,
            double leftRightPercent,
            double rightLeftPercent,
            double rightRightPercent) {
            m_leftLeft.store(mixCoefficient(leftLeftPercent), std::memory_order_relaxed);
            m_leftRight.store(mixCoefficient(leftRightPercent), std::memory_order_relaxed);
            m_rightLeft.store(mixCoefficient(rightLeftPercent), std::memory_order_relaxed);
            m_rightRight.store(mixCoefficient(rightRightPercent), std::memory_order_relaxed);
        }

    protected:
        qint64 processReading(const talcs::AudioSourceReadData &readData) override {
            const int channelCount = readData.buffer->channelCount();
            if (channelCount == 0) {
                return readData.length;
            }
            const float leftLeft = m_leftLeft.load(std::memory_order_relaxed);
            const float leftRight = m_leftRight.load(std::memory_order_relaxed);
            const float rightLeft = m_rightLeft.load(std::memory_order_relaxed);
            const float rightRight = m_rightRight.load(std::memory_order_relaxed);
            const bool hasRightInput = channelCount > 1;
            for (qint64 position = readData.startPos;
                 position < readData.startPos + readData.length; ++position) {
                const float leftInput = readData.buffer->sample(0, position);
                const float rightInput = hasRightInput
                    ? readData.buffer->sample(1, position)
                    : 0.0f;
                readData.buffer->setSample(0, position, leftInput * leftLeft + rightInput * leftRight);
                if (hasRightInput) {
                    readData.buffer->setSample(1, position, leftInput * rightLeft + rightInput * rightRight);
                }
            }
            return readData.length;
        }

    private:
        std::atomic<float> m_leftLeft{1.0f};
        std::atomic<float> m_leftRight{0.0f};
        std::atomic<float> m_rightLeft{0.0f};
        std::atomic<float> m_rightRight{1.0f};
    };

    ChannelMapperEffectsUnit::ChannelMapperEffectsUnit(QQmlComponent *editorComponent, QObject *parent)
        : EffectsUnit(parent) {
        auto processor = std::make_unique<ChannelMapperProcessor>();
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
            qFatal("ChannelMapperEditor must create a QQuickItem");
        }
        setEditor(editor);
    }

    ChannelMapperEffectsUnit::~ChannelMapperEffectsUnit() = default;

    double ChannelMapperEffectsUnit::leftLeftMixPercent() const {
        return m_leftLeftMixPercent;
    }

    double ChannelMapperEffectsUnit::leftRightMixPercent() const {
        return m_leftRightMixPercent;
    }

    double ChannelMapperEffectsUnit::rightLeftMixPercent() const {
        return m_rightLeftMixPercent;
    }

    double ChannelMapperEffectsUnit::rightRightMixPercent() const {
        return m_rightRightMixPercent;
    }

    QJsonValue ChannelMapperEffectsUnit::getState() const {
        return QJsonObject{
            {QStringLiteral("leftLeftMixPercent"), m_committedLeftLeftMixPercent},
            {QStringLiteral("leftRightMixPercent"), m_committedLeftRightMixPercent},
            {QStringLiteral("rightLeftMixPercent"), m_committedRightLeftMixPercent},
            {QStringLiteral("rightRightMixPercent"), m_committedRightRightMixPercent},
        };
    }

    void ChannelMapperEffectsUnit::setState(const QJsonValue &state) {
        const auto object = state.isObject() ? state.toObject() : QJsonObject{};
        const double leftLeft = normalizedMixPercent(object.value(QStringLiteral("leftLeftMixPercent")));
        const double leftRight = normalizedMixPercent(object.value(QStringLiteral("leftRightMixPercent")));
        const double rightLeft = normalizedMixPercent(object.value(QStringLiteral("rightLeftMixPercent")));
        const double rightRight = normalizedMixPercent(object.value(QStringLiteral("rightRightMixPercent")));
        const bool stateChanged = !mixValuesEqual(m_committedLeftLeftMixPercent, leftLeft)
            || !mixValuesEqual(m_committedLeftRightMixPercent, leftRight)
            || !mixValuesEqual(m_committedRightLeftMixPercent, rightLeft)
            || !mixValuesEqual(m_committedRightRightMixPercent, rightRight);
        const bool leftLeftChanged = !mixValuesEqual(m_leftLeftMixPercent, leftLeft);
        const bool leftRightChanged = !mixValuesEqual(m_leftRightMixPercent, leftRight);
        const bool rightLeftChanged = !mixValuesEqual(m_rightLeftMixPercent, rightLeft);
        const bool rightRightChanged = !mixValuesEqual(m_rightRightMixPercent, rightRight);

        m_committedLeftLeftMixPercent = leftLeft;
        m_committedLeftRightMixPercent = leftRight;
        m_committedRightLeftMixPercent = rightLeft;
        m_committedRightRightMixPercent = rightRight;
        m_leftLeftMixPercent = leftLeft;
        m_leftRightMixPercent = leftRight;
        m_rightLeftMixPercent = rightLeft;
        m_rightRightMixPercent = rightRight;
        updateProcessor();

        if (leftLeftChanged) {
            Q_EMIT leftLeftMixPercentChanged();
        }
        if (leftRightChanged) {
            Q_EMIT leftRightMixPercentChanged();
        }
        if (rightLeftChanged) {
            Q_EMIT rightLeftMixPercentChanged();
        }
        if (rightRightChanged) {
            Q_EMIT rightRightMixPercentChanged();
        }
        if (stateChanged) {
            Q_EMIT updated();
        }
    }

    void ChannelMapperEffectsUnit::previewLeftLeftMixPercent(double value) {
        if (previewMix(m_leftLeftMixPercent, value)) {
            Q_EMIT leftLeftMixPercentChanged();
        }
    }

    void ChannelMapperEffectsUnit::previewLeftRightMixPercent(double value) {
        if (previewMix(m_leftRightMixPercent, value)) {
            Q_EMIT leftRightMixPercentChanged();
        }
    }

    void ChannelMapperEffectsUnit::previewRightLeftMixPercent(double value) {
        if (previewMix(m_rightLeftMixPercent, value)) {
            Q_EMIT rightLeftMixPercentChanged();
        }
    }

    void ChannelMapperEffectsUnit::previewRightRightMixPercent(double value) {
        if (previewMix(m_rightRightMixPercent, value)) {
            Q_EMIT rightRightMixPercentChanged();
        }
    }

    void ChannelMapperEffectsUnit::commitPreview() {
        if (mixValuesEqual(m_committedLeftLeftMixPercent, m_leftLeftMixPercent)
            && mixValuesEqual(m_committedLeftRightMixPercent, m_leftRightMixPercent)
            && mixValuesEqual(m_committedRightLeftMixPercent, m_rightLeftMixPercent)
            && mixValuesEqual(m_committedRightRightMixPercent, m_rightRightMixPercent)) {
            return;
        }
        m_committedLeftLeftMixPercent = m_leftLeftMixPercent;
        m_committedLeftRightMixPercent = m_leftRightMixPercent;
        m_committedRightLeftMixPercent = m_rightLeftMixPercent;
        m_committedRightRightMixPercent = m_rightRightMixPercent;
        Q_EMIT updated();
    }

    void ChannelMapperEffectsUnit::setLeftLeftMixPercent(double value) {
        previewLeftLeftMixPercent(value);
        commitPreview();
    }

    void ChannelMapperEffectsUnit::setLeftRightMixPercent(double value) {
        previewLeftRightMixPercent(value);
        commitPreview();
    }

    void ChannelMapperEffectsUnit::setRightLeftMixPercent(double value) {
        previewRightLeftMixPercent(value);
        commitPreview();
    }

    void ChannelMapperEffectsUnit::setRightRightMixPercent(double value) {
        previewRightRightMixPercent(value);
        commitPreview();
    }

    bool ChannelMapperEffectsUnit::previewMix(double &member, double value) {
        value = std::clamp(value, minimumMixPercent, maximumMixPercent);
        if (mixValuesEqual(member, value)) {
            return false;
        }
        member = value;
        updateProcessor();
        return true;
    }

    void ChannelMapperEffectsUnit::updateProcessor() {
        m_processor->setMixCoefficients(m_leftLeftMixPercent, m_leftRightMixPercent,
                                        m_rightLeftMixPercent, m_rightRightMixPercent);
    }

    ChannelMapperEffectsUnitClass::ChannelMapperEffectsUnitClass(QObject *parent)
        : EffectsUnitClass(tr("Channel Mapper"), parent),
          m_editorComponent(new QQmlComponent(Core::RuntimeInterface::qmlEngine(),
                                              QStringLiteral("DiffScope.ChannelMapperEffectsUnit"),
                                              QStringLiteral("ChannelMapperEditor"), this)) {
        if (m_editorComponent->isError()) {
            qFatal() << m_editorComponent->errorString();
        }
    }

    ChannelMapperEffectsUnitClass::~ChannelMapperEffectsUnitClass() = default;

    EffectsUnitManager::EffectsUnit *ChannelMapperEffectsUnitClass::create(QObject *parent) const {
        return new ChannelMapperEffectsUnit(m_editorComponent, parent);
    }

}

#include "moc_ChannelMapperEffectsUnit.cpp"
