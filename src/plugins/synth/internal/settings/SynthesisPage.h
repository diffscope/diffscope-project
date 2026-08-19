// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_SYNTH_SYNTHESISPAGE_H
#define DIFFSCOPE_SYNTH_SYNTHESISPAGE_H

#include <QStringList>
#include <QVariant>

#include <CoreApi/isettingpage.h>

namespace Synth::Internal {

    class SynthesisPage final : public Core::ISettingPage {
        Q_OBJECT
        Q_PROPERTY(double paddingBase READ paddingBase WRITE setPaddingBase NOTIFY valuesChanged)
        Q_PROPERTY(double paddingAdditional READ paddingAdditional WRITE setPaddingAdditional NOTIFY valuesChanged)
        Q_PROPERTY(double paddingGap READ paddingGap WRITE setPaddingGap NOTIFY valuesChanged)
        Q_PROPERTY(QString restLyrics READ restLyrics WRITE setRestLyrics NOTIFY valuesChanged)
        Q_PROPERTY(int parameterSampleRate READ parameterSampleRate WRITE setParameterSampleRate NOTIFY valuesChanged)
        Q_PROPERTY(int mixSampleRate READ mixSampleRate WRITE setMixSampleRate NOTIFY valuesChanged)
        Q_PROPERTY(int cacheMaximumGiB READ cacheMaximumGiB WRITE setCacheMaximumGiB NOTIFY valuesChanged)
        Q_PROPERTY(int cacheExpiryDays READ cacheExpiryDays WRITE setCacheExpiryDays NOTIFY valuesChanged)
        Q_PROPERTY(int diagnosticsMaximumMiB READ diagnosticsMaximumMiB WRITE setDiagnosticsMaximumMiB NOTIFY valuesChanged)
        Q_PROPERTY(int diagnosticsExpiryDays READ diagnosticsExpiryDays WRITE setDiagnosticsExpiryDays NOTIFY valuesChanged)
        Q_PROPERTY(int audioDownloadMaximumMiB READ audioDownloadMaximumMiB WRITE setAudioDownloadMaximumMiB NOTIFY valuesChanged)
        Q_PROPERTY(int environmentTagTtlSeconds READ environmentTagTtlSeconds WRITE setEnvironmentTagTtlSeconds NOTIFY valuesChanged)

    public:
        explicit SynthesisPage(QObject *parent = nullptr);
        ~SynthesisPage() override;

        QString sortKeyword() const override;
        bool matches(const QString &word) override;
        QObject *widget() override;
        void beginSetting() override;
        bool accept() override;

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
        int diagnosticsMaximumMiB() const;
        void setDiagnosticsMaximumMiB(int value);
        int diagnosticsExpiryDays() const;
        void setDiagnosticsExpiryDays(int value);
        int audioDownloadMaximumMiB() const;
        void setAudioDownloadMaximumMiB(int value);
        int environmentTagTtlSeconds() const;
        void setEnvironmentTagTtlSeconds(int value);

        Q_INVOKABLE QVariantMap cacheSizes() const;
        Q_INVOKABLE void clearCache(const QStringList &taskTypes);
        Q_INVOKABLE void clearDiagnostics();

    Q_SIGNALS:
        void valuesChanged();

    private:
        QObject *m_widget{};
        double m_paddingBase{100.0};
        double m_paddingAdditional{100.0};
        double m_paddingGap{200.0};
        QString m_restLyrics{QStringLiteral("AP, SP")};
        int m_parameterSampleRate{100};
        int m_mixSampleRate{100};
        int m_cacheMaximumGiB{10};
        int m_cacheExpiryDays{30};
        int m_diagnosticsMaximumMiB{256};
        int m_diagnosticsExpiryDays{7};
        int m_audioDownloadMaximumMiB{512};
        int m_environmentTagTtlSeconds{60};
        bool m_loading{};
    };

}

#endif // DIFFSCOPE_SYNTH_SYNTHESISPAGE_H
