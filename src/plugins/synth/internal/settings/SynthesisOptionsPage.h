#ifndef DIFFSCOPE_SYNTH_SYNTHESISOPTIONSPAGE_H
#define DIFFSCOPE_SYNTH_SYNTHESISOPTIONSPAGE_H

#include <CoreApi/isettingpage.h>

class QAbstractItemModel;

namespace Synth::Internal {

    class ArchitectureExtraModel;

    class SynthesisOptionsPage final : public Core::ISettingPage {
        Q_OBJECT
        Q_PROPERTY(double paddingBase READ paddingBase WRITE setPaddingBase NOTIFY valuesChanged)
        Q_PROPERTY(double paddingAdditional READ paddingAdditional WRITE setPaddingAdditional NOTIFY valuesChanged)
        Q_PROPERTY(double paddingGap READ paddingGap WRITE setPaddingGap NOTIFY valuesChanged)
        Q_PROPERTY(QString restLyrics READ restLyrics WRITE setRestLyrics NOTIFY valuesChanged)
        Q_PROPERTY(int parameterSampleRate READ parameterSampleRate WRITE setParameterSampleRate NOTIFY valuesChanged)
        Q_PROPERTY(int mixSampleRate READ mixSampleRate WRITE setMixSampleRate NOTIFY valuesChanged)
        Q_PROPERTY(int cacheMaximumGiB READ cacheMaximumGiB WRITE setCacheMaximumGiB NOTIFY valuesChanged)
        Q_PROPERTY(int cacheExpiryDays READ cacheExpiryDays WRITE setCacheExpiryDays NOTIFY valuesChanged)
        Q_PROPERTY(int audioDownloadMaximumMiB READ audioDownloadMaximumMiB WRITE setAudioDownloadMaximumMiB NOTIFY valuesChanged)
        Q_PROPERTY(int environmentTagTtlSeconds READ environmentTagTtlSeconds WRITE setEnvironmentTagTtlSeconds NOTIFY valuesChanged)
        Q_PROPERTY(QAbstractItemModel *architectureExtraModel READ architectureExtraModel CONSTANT)
        Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)

    public:
        explicit SynthesisOptionsPage(QObject *parent = nullptr);
        ~SynthesisOptionsPage() override;

        QString sortKeyword() const override;
        bool matches(const QString &word) override;
        QObject *widget() override;
        void beginSetting() override;
        bool accept() override;
        void endSetting() override;

        double paddingBase() const;
        void setPaddingBase(double value);
        double paddingAdditional() const;
        void setPaddingAdditional(double value);
        double paddingGap() const;
        void setPaddingGap(double value);
        QString restLyrics() const;
        void setRestLyrics(const QString &value);
        int parameterSampleRate() const;
        void setParameterSampleRate(int value);
        int mixSampleRate() const;
        void setMixSampleRate(int value);
        int cacheMaximumGiB() const;
        void setCacheMaximumGiB(int value);
        int cacheExpiryDays() const;
        void setCacheExpiryDays(int value);
        int audioDownloadMaximumMiB() const;
        void setAudioDownloadMaximumMiB(int value);
        int environmentTagTtlSeconds() const;
        void setEnvironmentTagTtlSeconds(int value);
        QAbstractItemModel *architectureExtraModel() const;
        QString errorMessage() const;

        Q_INVOKABLE void clearCache();

    Q_SIGNALS:
        void valuesChanged();
        void errorMessageChanged();

    private:
        void setErrorMessage(const QString &message);

        ArchitectureExtraModel *m_architectureExtras{};
        QObject *m_widget{};
        double m_paddingBase{100.0};
        double m_paddingAdditional{100.0};
        double m_paddingGap{1000.0};
        QString m_restLyrics{QStringLiteral("AP, SP")};
        int m_parameterSampleRate{100};
        int m_mixSampleRate{100};
        int m_cacheMaximumGiB{10};
        int m_cacheExpiryDays{30};
        int m_audioDownloadMaximumMiB{512};
        int m_environmentTagTtlSeconds{60};
        QString m_errorMessage;
        bool m_loading{};
    };

}

#endif // DIFFSCOPE_SYNTH_SYNTHESISOPTIONSPAGE_H
