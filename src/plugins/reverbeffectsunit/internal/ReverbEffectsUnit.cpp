// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "ReverbEffectsUnit.h"

#include <algorithm>
#include <cmath>
#include <memory>

#include <QJsonObject>
#include <QQmlComponent>
#include <QQuickItem>
#include <QVariant>

#include <CoreApi/runtimeinterface.h>

#include <reverbeffectsunit/internal/ReverbParameters.h>
#include <reverbeffectsunit/internal/ReverbProcessor.h>

namespace ReverbEffectsUnit::Internal {

    namespace {

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

    }

    ReverbEffectsUnit::ReverbEffectsUnit(QQmlComponent *editorComponent,
                                         QObject *parent)
        : EffectsUnit(parent),
          m_committedSizeMilliseconds(defaultSizeMilliseconds),
          m_committedDecaySeconds(defaultDecaySeconds),
          m_committedDampingPercent(defaultDampingPercent),
          m_committedPreDelayMilliseconds(defaultPreDelayMilliseconds),
          m_committedMixPercent(defaultMixPercent),
          m_sizeMilliseconds(defaultSizeMilliseconds),
          m_decaySeconds(defaultDecaySeconds),
          m_dampingPercent(defaultDampingPercent),
          m_preDelayMilliseconds(defaultPreDelayMilliseconds),
          m_mixPercent(defaultMixPercent) {
        auto processor = std::make_unique<ReverbProcessor>();
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
            qFatal("ReverbEditor must create a QQuickItem");
        }
        setEditor(editor);
    }

    ReverbEffectsUnit::~ReverbEffectsUnit() = default;

    double ReverbEffectsUnit::sizeMilliseconds() const {
        return m_sizeMilliseconds;
    }

    double ReverbEffectsUnit::decaySeconds() const {
        return m_decaySeconds;
    }

    double ReverbEffectsUnit::dampingPercent() const {
        return m_dampingPercent;
    }

    double ReverbEffectsUnit::preDelayMilliseconds() const {
        return m_preDelayMilliseconds;
    }

    double ReverbEffectsUnit::mixPercent() const {
        return m_mixPercent;
    }

    QJsonValue ReverbEffectsUnit::getState() const {
        return QJsonObject{
            {QStringLiteral("sizeMilliseconds"), m_committedSizeMilliseconds},
            {QStringLiteral("decaySeconds"), m_committedDecaySeconds},
            {QStringLiteral("dampingPercent"), m_committedDampingPercent},
            {QStringLiteral("preDelayMilliseconds"),
             m_committedPreDelayMilliseconds},
            {QStringLiteral("mixPercent"), m_committedMixPercent},
        };
    }

    void ReverbEffectsUnit::setState(const QJsonValue &state) {
        const auto object = state.isObject() ? state.toObject() : QJsonObject{};
        const double size = normalizedValue(
            object.value(QStringLiteral("sizeMilliseconds")),
            defaultSizeMilliseconds,
            minimumSizeMilliseconds, maximumSizeMilliseconds);
        const double decay = normalizedValue(
            object.value(QStringLiteral("decaySeconds")),
            defaultDecaySeconds,
            minimumDecaySeconds, maximumDecaySeconds);
        const double damping = normalizedValue(
            object.value(QStringLiteral("dampingPercent")),
            defaultDampingPercent,
            minimumDampingPercent, maximumDampingPercent);
        const double preDelay = normalizedValue(
            object.value(QStringLiteral("preDelayMilliseconds")),
            defaultPreDelayMilliseconds,
            minimumPreDelayMilliseconds, maximumPreDelayMilliseconds);
        const double mix = normalizedValue(
            object.value(QStringLiteral("mixPercent")),
            defaultMixPercent,
            minimumMixPercent, maximumMixPercent);

        const bool stateChanged =
            !valuesEqual(m_committedSizeMilliseconds, size)
            || !valuesEqual(m_committedDecaySeconds, decay)
            || !valuesEqual(m_committedDampingPercent, damping)
            || !valuesEqual(m_committedPreDelayMilliseconds, preDelay)
            || !valuesEqual(m_committedMixPercent, mix);
        const bool sizeChanged = !valuesEqual(m_sizeMilliseconds, size);
        const bool decayChanged = !valuesEqual(m_decaySeconds, decay);
        const bool dampingChanged = !valuesEqual(m_dampingPercent, damping);
        const bool preDelayChanged =
            !valuesEqual(m_preDelayMilliseconds, preDelay);
        const bool mixChanged = !valuesEqual(m_mixPercent, mix);

        m_committedSizeMilliseconds = size;
        m_committedDecaySeconds = decay;
        m_committedDampingPercent = damping;
        m_committedPreDelayMilliseconds = preDelay;
        m_committedMixPercent = mix;
        m_sizeMilliseconds = size;
        m_decaySeconds = decay;
        m_dampingPercent = damping;
        m_preDelayMilliseconds = preDelay;
        m_mixPercent = mix;
        updateProcessor();

        if (sizeChanged) {
            Q_EMIT sizeMillisecondsChanged();
        }
        if (decayChanged) {
            Q_EMIT decaySecondsChanged();
        }
        if (dampingChanged) {
            Q_EMIT dampingPercentChanged();
        }
        if (preDelayChanged) {
            Q_EMIT preDelayMillisecondsChanged();
        }
        if (mixChanged) {
            Q_EMIT mixPercentChanged();
        }
        if (stateChanged) {
            Q_EMIT updated();
        }
    }

    void ReverbEffectsUnit::refresh() {
        m_processor->refresh();
    }

    void ReverbEffectsUnit::previewSizeMilliseconds(double value) {
        if (previewValue(m_sizeMilliseconds, value,
                         minimumSizeMilliseconds, maximumSizeMilliseconds)) {
            Q_EMIT sizeMillisecondsChanged();
        }
    }

    void ReverbEffectsUnit::previewDecaySeconds(double value) {
        if (previewValue(m_decaySeconds, value,
                         minimumDecaySeconds, maximumDecaySeconds)) {
            Q_EMIT decaySecondsChanged();
        }
    }

    void ReverbEffectsUnit::previewDampingPercent(double value) {
        if (previewValue(m_dampingPercent, value,
                         minimumDampingPercent, maximumDampingPercent)) {
            Q_EMIT dampingPercentChanged();
        }
    }

    void ReverbEffectsUnit::previewPreDelayMilliseconds(double value) {
        if (previewValue(m_preDelayMilliseconds, value,
                         minimumPreDelayMilliseconds,
                         maximumPreDelayMilliseconds)) {
            Q_EMIT preDelayMillisecondsChanged();
        }
    }

    void ReverbEffectsUnit::previewMixPercent(double value) {
        if (previewValue(m_mixPercent, value,
                         minimumMixPercent, maximumMixPercent)) {
            Q_EMIT mixPercentChanged();
        }
    }

    void ReverbEffectsUnit::commitPreview() {
        if (valuesEqual(m_committedSizeMilliseconds, m_sizeMilliseconds)
            && valuesEqual(m_committedDecaySeconds, m_decaySeconds)
            && valuesEqual(m_committedDampingPercent, m_dampingPercent)
            && valuesEqual(m_committedPreDelayMilliseconds,
                           m_preDelayMilliseconds)
            && valuesEqual(m_committedMixPercent, m_mixPercent)) {
            return;
        }
        m_committedSizeMilliseconds = m_sizeMilliseconds;
        m_committedDecaySeconds = m_decaySeconds;
        m_committedDampingPercent = m_dampingPercent;
        m_committedPreDelayMilliseconds = m_preDelayMilliseconds;
        m_committedMixPercent = m_mixPercent;
        Q_EMIT updated();
    }

    void ReverbEffectsUnit::setSizeMilliseconds(double value) {
        previewSizeMilliseconds(value);
        commitPreview();
    }

    void ReverbEffectsUnit::setDecaySeconds(double value) {
        previewDecaySeconds(value);
        commitPreview();
    }

    void ReverbEffectsUnit::setDampingPercent(double value) {
        previewDampingPercent(value);
        commitPreview();
    }

    void ReverbEffectsUnit::setPreDelayMilliseconds(double value) {
        previewPreDelayMilliseconds(value);
        commitPreview();
    }

    void ReverbEffectsUnit::setMixPercent(double value) {
        previewMixPercent(value);
        commitPreview();
    }

    bool ReverbEffectsUnit::previewValue(double &member, double value,
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

    void ReverbEffectsUnit::updateProcessor() {
        m_processor->setParameters(m_sizeMilliseconds, m_decaySeconds,
                                   m_dampingPercent, m_preDelayMilliseconds,
                                   m_mixPercent);
    }

    ReverbEffectsUnitClass::ReverbEffectsUnitClass(QObject *parent)
        : EffectsUnitClass(tr("Reverb"), parent),
          m_editorComponent(new QQmlComponent(
              Core::RuntimeInterface::qmlEngine(),
              QStringLiteral("DiffScope.ReverbEffectsUnit"),
              QStringLiteral("ReverbEditor"), this)) {
        if (m_editorComponent->isError()) {
            qFatal() << m_editorComponent->errorString();
        }
    }

    ReverbEffectsUnitClass::~ReverbEffectsUnitClass() = default;

    Audio::EffectsUnit *ReverbEffectsUnitClass::create(
        QObject *parent) const {
        return new ReverbEffectsUnit(m_editorComponent, parent);
    }

}

#include "moc_ReverbEffectsUnit.cpp"
