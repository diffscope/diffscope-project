// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_AUDIOPLUGIN_PLAYBACKPAGE_H
#define DIFFSCOPE_AUDIOPLUGIN_PLAYBACKPAGE_H

#include <CoreApi/isettingpage.h>

namespace Audio::Internal {

    class PlaybackPage : public Core::ISettingPage {
        Q_OBJECT
        Q_PROPERTY(bool metronomeEnabled READ metronomeEnabled WRITE setMetronomeEnabled NOTIFY metronomeEnabledChanged)
        Q_PROPERTY(double metronomeGain READ metronomeGain WRITE setMetronomeGain NOTIFY metronomeGainChanged)
        Q_PROPERTY(double metronomePan READ metronomePan WRITE setMetronomePan NOTIFY metronomePanChanged)
    public:
        explicit PlaybackPage(QObject *parent = nullptr);
        ~PlaybackPage() override;

        bool matches(const QString &word) override;
        QString sortKeyword() const override;
        QObject *widget() override;
        void beginSetting() override;
        bool accept() override;
        void endSetting() override;

        bool metronomeEnabled() const;
        void setMetronomeEnabled(bool enabled);
        double metronomeGain() const;
        void setMetronomeGain(double gain);
        double metronomePan() const;
        void setMetronomePan(double pan);

    Q_SIGNALS:
        void metronomeEnabledChanged(bool enabled);
        void metronomeGainChanged(double gain);
        void metronomePanChanged(double pan);

    private:
        void initializeMetronomeProperties();
        bool widgetMatches(const QString &word);
        QObject *m_widget{};
        bool m_metronomePropertiesInitialized{};
    };

}

#endif // DIFFSCOPE_AUDIOPLUGIN_PLAYBACKPAGE_H
