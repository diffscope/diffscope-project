// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_EQUALIZER_EFFECTS_UNIT_EQUALIZEREFFECTSUNIT_H
#define DIFFSCOPE_EQUALIZER_EFFECTS_UNIT_EQUALIZEREFFECTSUNIT_H

#include <array>

#include <QElapsedTimer>
#include <QJsonValue>
#include <qqmlintegration.h>

#include <TalcsCore/SmoothedFloat.h>

#include <audio/EffectsUnit.h>
#include <audio/EffectsUnitClass.h>

#include <equalizereffectsunit/internal/EqualizerParameters.h>

class QAbstractItemModel;
class QQmlComponent;
class QTimer;

namespace EqualizerEffectsUnit::Internal {

    class EqualizerBandModel;
    class EqualizerProcessor;

    class EqualizerEffectsUnit : public Audio::EffectsUnit {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_PROPERTY(QAbstractItemModel *bands READ bands CONSTANT)
        Q_PROPERTY(int bandCount READ bandCount NOTIFY bandCountChanged)
        Q_PROPERTY(bool canAddBand READ canAddBand NOTIFY bandCountChanged)
        Q_PROPERTY(int currentIndex READ currentIndex NOTIFY currentIndexChanged)
        Q_PROPERTY(bool hasCurrentBand READ hasCurrentBand NOTIFY currentBandChanged)
        Q_PROPERTY(double currentFrequencyHz READ currentFrequencyHz NOTIFY currentBandChanged)
        Q_PROPERTY(double currentGainDb READ currentGainDb NOTIFY currentBandChanged)
        Q_PROPERTY(double currentQ READ currentQ NOTIFY currentBandChanged)
        Q_PROPERTY(BandType currentType READ currentType NOTIFY currentBandChanged)

    public:
        enum BandType {
            Bell = static_cast<int>(EqualizerBandType::Bell),
            LowShelf = static_cast<int>(EqualizerBandType::LowShelf),
            HighShelf = static_cast<int>(EqualizerBandType::HighShelf),
        };
        Q_ENUM(BandType)

        explicit EqualizerEffectsUnit(QQmlComponent *editorComponent,
                                      QObject *parent = nullptr);
        ~EqualizerEffectsUnit() override;

        QAbstractItemModel *bands() const;
        int bandCount() const;
        bool canAddBand() const;
        int currentIndex() const;
        bool hasCurrentBand() const;
        double currentFrequencyHz() const;
        double currentGainDb() const;
        double currentQ() const;
        BandType currentType() const;

        QJsonValue getState() const override;
        void setState(const QJsonValue &state) override;
        void refresh() override;

        Q_INVOKABLE void selectBand(int index);
        Q_INVOKABLE void selectPreviousBand();
        Q_INVOKABLE void selectNextBand();
        Q_INVOKABLE void addBand();
        Q_INVOKABLE void addBandAt(double frequencyHz, double gainDb);
        Q_INVOKABLE void removeBand(int index);
        Q_INVOKABLE void removeCurrentBand();
        Q_INVOKABLE void previewBandPosition(int index, double frequencyHz,
                                             double gainDb);
        Q_INVOKABLE void previewBandQ(int index, double value);
        Q_INVOKABLE void previewCurrentFrequencyHz(double value);
        Q_INVOKABLE void previewCurrentGainDb(double value);
        Q_INVOKABLE void previewCurrentQ(double value);
        Q_INVOKABLE void commitPreview();
        Q_INVOKABLE void setCurrentFrequencyHz(double value);
        Q_INVOKABLE void setCurrentGainDb(double value);
        Q_INVOKABLE void setCurrentQ(double value);
        Q_INVOKABLE void setCurrentType(BandType type);

        const std::array<float, equalizerResponsePointCount> &responseCurveDb() const;
        const std::array<float, equalizerSpectrumBinCount> &spectrumCurveDb() const;

    Q_SIGNALS:
        void bandCountChanged();
        void currentIndexChanged();
        void currentBandChanged();
        void responseCurveChanged();
        void spectrumCurveChanged();

    private:
        const EqualizerBand *currentBand() const;
        void setCurrentIndex(int index);
        QList<int> frequencyOrderedIndices() const;
        int medianFrequencyBandIndex() const;
        double largestFrequencyGapMidpoint() const;
        bool previewBand(int index, const EqualizerBand &band);
        void updateProcessor();
        void rebuildResponseCurve();

        void updateSpectrumTimer();
        void startSpectrumTimer();
        void stopSpectrumTimer();
        void tickSpectrum();
        void resetSpectrumDisplay();

        EqualizerBandModel *m_bandModel{};
        EqualizerProcessor *m_processor{};
        QTimer *m_spectrumTimer{};
        QElapsedTimer m_spectrumTickTime;
        QElapsedTimer m_lastSpectrumFrameTime;
        std::array<talcs::SmoothedFloat, equalizerSpectrumBinCount> m_smoothedSpectrum;
        std::array<float, equalizerSpectrumBinCount> m_spectrumCurve;
        std::array<float, equalizerResponsePointCount> m_responseCurve;
        EqualizerBandList m_committedBands;
        double m_responseSampleRate{44100.0};
        int m_currentIndex{1};
        bool m_spectrumActive{};
        bool m_spectrumDecayStarted{};
    };

    class EqualizerEffectsUnitClass : public Audio::EffectsUnitClass {
        Q_OBJECT

    public:
        explicit EqualizerEffectsUnitClass(QObject *parent = nullptr);
        ~EqualizerEffectsUnitClass() override;

        Audio::EffectsUnit *create(QObject *parent = nullptr) const override;

    private:
        QQmlComponent *m_editorComponent{};
    };

}

#endif // DIFFSCOPE_EQUALIZER_EFFECTS_UNIT_EQUALIZEREFFECTSUNIT_H
