// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_REVERB_EFFECTS_UNIT_REVERBEFFECTSUNIT_H
#define DIFFSCOPE_REVERB_EFFECTS_UNIT_REVERBEFFECTSUNIT_H

#include <QJsonValue>
#include <qqmlintegration.h>

#include <audio/EffectsUnit.h>
#include <audio/EffectsUnitClass.h>

class QQmlComponent;

namespace ReverbEffectsUnit::Internal {

    class ReverbProcessor;

    class ReverbEffectsUnit : public Audio::EffectsUnit {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_PROPERTY(double sizeMilliseconds READ sizeMilliseconds NOTIFY sizeMillisecondsChanged)
        Q_PROPERTY(double decaySeconds READ decaySeconds NOTIFY decaySecondsChanged)
        Q_PROPERTY(double dampingPercent READ dampingPercent NOTIFY dampingPercentChanged)
        Q_PROPERTY(double preDelayMilliseconds READ preDelayMilliseconds NOTIFY preDelayMillisecondsChanged)
        Q_PROPERTY(double mixPercent READ mixPercent NOTIFY mixPercentChanged)

    public:
        explicit ReverbEffectsUnit(QQmlComponent *editorComponent,
                                   QObject *parent = nullptr);
        ~ReverbEffectsUnit() override;

        double sizeMilliseconds() const;
        double decaySeconds() const;
        double dampingPercent() const;
        double preDelayMilliseconds() const;
        double mixPercent() const;

        QJsonValue getState() const override;
        void setState(const QJsonValue &state) override;
        void refresh() override;

        Q_INVOKABLE void previewSizeMilliseconds(double value);
        Q_INVOKABLE void previewDecaySeconds(double value);
        Q_INVOKABLE void previewDampingPercent(double value);
        Q_INVOKABLE void previewPreDelayMilliseconds(double value);
        Q_INVOKABLE void previewMixPercent(double value);
        Q_INVOKABLE void commitPreview();
        Q_INVOKABLE void setSizeMilliseconds(double value);
        Q_INVOKABLE void setDecaySeconds(double value);
        Q_INVOKABLE void setDampingPercent(double value);
        Q_INVOKABLE void setPreDelayMilliseconds(double value);
        Q_INVOKABLE void setMixPercent(double value);

    Q_SIGNALS:
        void sizeMillisecondsChanged();
        void decaySecondsChanged();
        void dampingPercentChanged();
        void preDelayMillisecondsChanged();
        void mixPercentChanged();

    private:
        bool previewValue(double &member, double value,
                          double minimum, double maximum);
        void updateProcessor();

        ReverbProcessor *m_processor{};
        double m_committedSizeMilliseconds;
        double m_committedDecaySeconds;
        double m_committedDampingPercent;
        double m_committedPreDelayMilliseconds;
        double m_committedMixPercent;
        double m_sizeMilliseconds;
        double m_decaySeconds;
        double m_dampingPercent;
        double m_preDelayMilliseconds;
        double m_mixPercent;
    };

    class ReverbEffectsUnitClass : public Audio::EffectsUnitClass {
        Q_OBJECT

    public:
        explicit ReverbEffectsUnitClass(QObject *parent = nullptr);
        ~ReverbEffectsUnitClass() override;

        Audio::EffectsUnit *create(QObject *parent = nullptr) const override;

    private:
        QQmlComponent *m_editorComponent{};
    };

}

#endif // DIFFSCOPE_REVERB_EFFECTS_UNIT_REVERBEFFECTSUNIT_H
