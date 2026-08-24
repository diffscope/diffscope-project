// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_COMPRESSOR_EFFECTS_UNIT_COMPRESSOREFFECTSUNIT_H
#define DIFFSCOPE_COMPRESSOR_EFFECTS_UNIT_COMPRESSOREFFECTSUNIT_H

#include <QElapsedTimer>
#include <QJsonValue>
#include <qqmlintegration.h>

#include <TalcsCore/SmoothedFloat.h>

#include <effectsunitmanager/EffectsUnit.h>
#include <effectsunitmanager/EffectsUnitClass.h>

class QQmlComponent;
class QTimer;

namespace CompressorEffectsUnit::Internal {

    class CompressorProcessor;

    class CompressorEffectsUnit : public EffectsUnitManager::EffectsUnit {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_PROPERTY(double thresholdDb READ thresholdDb NOTIFY thresholdDbChanged)
        Q_PROPERTY(double ratio READ ratio NOTIFY ratioChanged)
        Q_PROPERTY(double attackMilliseconds READ attackMilliseconds NOTIFY attackMillisecondsChanged)
        Q_PROPERTY(double releaseMilliseconds READ releaseMilliseconds NOTIFY releaseMillisecondsChanged)
        Q_PROPERTY(double inputLevelDb READ inputLevelDb NOTIFY levelsChanged)
        Q_PROPERTY(double leftOutputLevelDb READ leftOutputLevelDb NOTIFY levelsChanged)
        Q_PROPERTY(double rightOutputLevelDb READ rightOutputLevelDb NOTIFY levelsChanged)
        Q_PROPERTY(double leftGainReductionDb READ leftGainReductionDb NOTIFY levelsChanged)
        Q_PROPERTY(double rightGainReductionDb READ rightGainReductionDb NOTIFY levelsChanged)

    public:
        explicit CompressorEffectsUnit(QQmlComponent *editorComponent, QObject *parent = nullptr);
        ~CompressorEffectsUnit() override;

        double thresholdDb() const;
        double ratio() const;
        double attackMilliseconds() const;
        double releaseMilliseconds() const;

        double inputLevelDb() const;
        double leftOutputLevelDb() const;
        double rightOutputLevelDb() const;
        double leftGainReductionDb() const;
        double rightGainReductionDb() const;

        QJsonValue getState() const override;
        void setState(const QJsonValue &state) override;

        Q_INVOKABLE void previewThresholdDb(double value);
        Q_INVOKABLE void previewRatio(double value);
        Q_INVOKABLE void previewAttackMilliseconds(double value);
        Q_INVOKABLE void previewReleaseMilliseconds(double value);
        Q_INVOKABLE void commitPreview();
        Q_INVOKABLE void setThresholdDb(double value);
        Q_INVOKABLE void setRatio(double value);
        Q_INVOKABLE void setAttackMilliseconds(double value);
        Q_INVOKABLE void setReleaseMilliseconds(double value);

    Q_SIGNALS:
        void thresholdDbChanged();
        void ratioChanged();
        void attackMillisecondsChanged();
        void releaseMillisecondsChanged();
        void levelsChanged();

    private:
        bool previewValue(double &member, double value, double minimum, double maximum);
        void updateProcessor();
        void updateMeterTimer();
        void startMeterTimer();
        void stopMeterTimer();
        void tickMeters();
        void resetMeterDisplays();

        CompressorProcessor *m_processor{};
        QTimer *m_meterTimer{};
        QElapsedTimer m_meterTickTime;
        QElapsedTimer m_lastMeterValueTime;
        talcs::SmoothedFloat m_inputLevel;
        talcs::SmoothedFloat m_leftOutputLevel;
        talcs::SmoothedFloat m_rightOutputLevel;
        talcs::SmoothedFloat m_leftGainReduction;
        talcs::SmoothedFloat m_rightGainReduction;
        double m_committedThresholdDb;
        double m_committedRatio;
        double m_committedAttackMilliseconds;
        double m_committedReleaseMilliseconds;
        double m_thresholdDb;
        double m_ratio;
        double m_attackMilliseconds;
        double m_releaseMilliseconds;
        bool m_meterActive{};
        bool m_meterDecayStarted{};
    };

    class CompressorEffectsUnitClass : public EffectsUnitManager::EffectsUnitClass {
        Q_OBJECT

    public:
        explicit CompressorEffectsUnitClass(QObject *parent = nullptr);
        ~CompressorEffectsUnitClass() override;

        EffectsUnitManager::EffectsUnit *create(QObject *parent = nullptr) const override;

    private:
        QQmlComponent *m_editorComponent{};
    };

}

#endif // DIFFSCOPE_COMPRESSOR_EFFECTS_UNIT_COMPRESSOREFFECTSUNIT_H
