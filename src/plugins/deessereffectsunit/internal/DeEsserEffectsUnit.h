// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_DEESSER_EFFECTS_UNIT_DEESSEREFFECTSUNIT_H
#define DIFFSCOPE_DEESSER_EFFECTS_UNIT_DEESSEREFFECTSUNIT_H

#include <array>

#include <QElapsedTimer>
#include <QJsonValue>
#include <qqmlintegration.h>

#include <TalcsCore/SmoothedFloat.h>

#include <audio/EffectsUnit.h>
#include <audio/EffectsUnitClass.h>

#include <deessereffectsunit/internal/DeEsserParameters.h>

class QQmlComponent;
class QTimer;

namespace DeEsserEffectsUnit::Internal {

    class DeEsserProcessor;

    class DeEsserEffectsUnit : public Audio::EffectsUnit {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_PROPERTY(double frequencyHz READ frequencyHz NOTIFY frequencyHzChanged)
        Q_PROPERTY(double bandwidthHz READ bandwidthHz NOTIFY bandwidthHzChanged)
        Q_PROPERTY(double thresholdDb READ thresholdDb NOTIFY thresholdDbChanged)
        Q_PROPERTY(bool outputSibilanceOnly READ outputSibilanceOnly NOTIFY outputSibilanceOnlyChanged)
        Q_PROPERTY(double leftBandLevelDb READ leftBandLevelDb NOTIFY levelsChanged)
        Q_PROPERTY(double rightBandLevelDb READ rightBandLevelDb NOTIFY levelsChanged)
        Q_PROPERTY(double leftGainReductionDb READ leftGainReductionDb NOTIFY levelsChanged)
        Q_PROPERTY(double rightGainReductionDb READ rightGainReductionDb NOTIFY levelsChanged)

    public:
        explicit DeEsserEffectsUnit(QQmlComponent *editorComponent,
                                    QObject *parent = nullptr);
        ~DeEsserEffectsUnit() override;

        double frequencyHz() const;
        double bandwidthHz() const;
        double thresholdDb() const;
        bool outputSibilanceOnly() const;

        double leftBandLevelDb() const;
        double rightBandLevelDb() const;
        double leftGainReductionDb() const;
        double rightGainReductionDb() const;

        QJsonValue getState() const override;
        void setState(const QJsonValue &state) override;
        void refresh() override;

        Q_INVOKABLE void previewFrequencyHz(double value);
        Q_INVOKABLE void previewBandwidthHz(double value);
        Q_INVOKABLE void previewThresholdDb(double value);
        Q_INVOKABLE void previewBandEdges(double leftFrequencyHz,
                                          double rightFrequencyHz);
        Q_INVOKABLE void commitPreview();
        Q_INVOKABLE void setFrequencyHz(double value);
        Q_INVOKABLE void setBandwidthHz(double value);
        Q_INVOKABLE void setThresholdDb(double value);
        Q_INVOKABLE void setOutputSibilanceOnly(bool enabled);

        const std::array<float, deEsserSpectrumBinCount> &spectrumCurveDb() const;

    Q_SIGNALS:
        void frequencyHzChanged();
        void bandwidthHzChanged();
        void thresholdDbChanged();
        void outputSibilanceOnlyChanged();
        void levelsChanged();
        void spectrumCurveChanged();

    private:
        bool previewValue(double &member, double value,
                          double minimum, double maximum);
        void updateProcessor();
        void updateAnalysisTimer();
        void startAnalysisTimer();
        void stopAnalysisTimer();
        void tickAnalysis();
        void resetLevelDisplays();
        void resetSpectrumDisplay();

        DeEsserProcessor *m_processor{};
        QTimer *m_analysisTimer{};
        QElapsedTimer m_analysisTickTime;
        QElapsedTimer m_lastMeterValueTime;
        QElapsedTimer m_lastSpectrumFrameTime;
        talcs::SmoothedFloat m_leftBandLevel;
        talcs::SmoothedFloat m_rightBandLevel;
        talcs::SmoothedFloat m_leftGainReduction;
        talcs::SmoothedFloat m_rightGainReduction;
        std::array<talcs::SmoothedFloat, deEsserSpectrumBinCount> m_smoothedSpectrum;
        std::array<float, deEsserSpectrumBinCount> m_spectrumCurve;
        double m_committedFrequencyHz;
        double m_committedBandwidthHz;
        double m_committedThresholdDb;
        double m_frequencyHz;
        double m_bandwidthHz;
        double m_thresholdDb;
        bool m_outputSibilanceOnly;
        bool m_analysisActive{};
        bool m_meterDecayStarted{};
        bool m_spectrumDecayStarted{};
    };

    class DeEsserEffectsUnitClass : public Audio::EffectsUnitClass {
        Q_OBJECT

    public:
        explicit DeEsserEffectsUnitClass(QObject *parent = nullptr);
        ~DeEsserEffectsUnitClass() override;

        Audio::EffectsUnit *create(QObject *parent = nullptr) const override;

    private:
        QQmlComponent *m_editorComponent{};
    };

}

#endif // DIFFSCOPE_DEESSER_EFFECTS_UNIT_DEESSEREFFECTSUNIT_H
