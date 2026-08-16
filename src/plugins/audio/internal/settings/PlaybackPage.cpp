// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "PlaybackPage.h"

#include <QLoggingCategory>
#include <QQmlComponent>

#include <CoreApi/runtimeinterface.h>

#include <audio/GlobalAudioContext.h>
#include <audio/internal/AudioPreference.h>

namespace Audio::Internal {

    Q_STATIC_LOGGING_CATEGORY(lcPlaybackPage, "diffscope.audio.playbackpage")

    PlaybackPage::PlaybackPage(QObject *parent) : Core::ISettingPage("audio.Playback", parent) {
        setTitle(tr("Playback"));
        setDescription(tr("Configure playback behavior"));
    }

    PlaybackPage::~PlaybackPage() {
        delete m_widget;
    }

    bool PlaybackPage::matches(const QString &word) {
        return ISettingPage::matches(word) || widgetMatches(word);
    }

    QString PlaybackPage::sortKeyword() const {
        return QStringLiteral("Playback");
    }

    QObject *PlaybackPage::widget() {
        initializeMetronomeProperties();
        if (m_widget)
            return m_widget;
        qCDebug(lcPlaybackPage) << "Creating widget";
        QQmlComponent component(Core::RuntimeInterface::qmlEngine(), "DiffScope.Audio", "PlaybackPage");
        if (component.isError()) {
            qFatal() << component.errorString();
        }
        m_widget = component.createWithInitialProperties({{"pageHandle", QVariant::fromValue(this)}});
        m_widget->setParent(this);
        return m_widget;
    }

    void PlaybackPage::beginSetting() {
        qCInfo(lcPlaybackPage) << "Beginning setting";
        widget();
        m_widget->setProperty("playbackBehavior", AudioPreference::instance()->property("playbackBehavior"));
        qCDebug(lcPlaybackPage) << "playbackBehavior" << m_widget->property("playbackBehavior");
        m_widget->setProperty("playbackTogglingAction", AudioPreference::instance()->property("playbackTogglingAction"));
        qCDebug(lcPlaybackPage) << "playbackTogglingAction" << m_widget->property("playbackTogglingAction");
        m_widget->setProperty("started", true);
        ISettingPage::beginSetting();
    }

    bool PlaybackPage::accept() {
        qCInfo(lcPlaybackPage) << "Accepting";
        qCDebug(lcPlaybackPage) << "playbackBehavior" << m_widget->property("playbackBehavior");
        AudioPreference::instance()->setProperty("playbackBehavior", m_widget->property("playbackBehavior"));
        qCDebug(lcPlaybackPage) << "playbackTogglingAction" << m_widget->property("playbackTogglingAction");
        AudioPreference::instance()->setProperty("playbackTogglingAction", m_widget->property("playbackTogglingAction"));
        AudioPreference::instance()->save();
        return ISettingPage::accept();
    }

    void PlaybackPage::endSetting() {
        qCInfo(lcPlaybackPage) << "Ending setting";
        m_widget->setProperty("started", false);
        ISettingPage::endSetting();
    }

    bool PlaybackPage::metronomeEnabled() const {
        return GlobalAudioContext::metronomeEnabled();
    }

    void PlaybackPage::setMetronomeEnabled(bool enabled) {
        GlobalAudioContext::setMetronomeEnabled(enabled);
    }

    double PlaybackPage::metronomeGain() const {
        return GlobalAudioContext::metronomeGain();
    }

    void PlaybackPage::setMetronomeGain(double gain) {
        GlobalAudioContext::setMetronomeGain(gain);
    }

    double PlaybackPage::metronomePan() const {
        return GlobalAudioContext::metronomePan();
    }

    void PlaybackPage::setMetronomePan(double pan) {
        GlobalAudioContext::setMetronomePan(pan);
    }

    void PlaybackPage::initializeMetronomeProperties() {
        if (m_metronomePropertiesInitialized) {
            return;
        }
        auto context = GlobalAudioContext::instance();
        if (!context) {
            return;
        }

        connect(context, &GlobalAudioContext::metronomeEnabledChanged,
                this, &PlaybackPage::metronomeEnabledChanged);
        connect(context, &GlobalAudioContext::metronomeGainChanged,
                this, &PlaybackPage::metronomeGainChanged);
        connect(context, &GlobalAudioContext::metronomePanChanged,
                this, &PlaybackPage::metronomePanChanged);
        m_metronomePropertiesInitialized = true;
    }

    bool PlaybackPage::widgetMatches(const QString &word) {
        widget();
        auto matcher = m_widget->property("matcher").value<QObject *>();
        bool ret = false;
        QMetaObject::invokeMethod(matcher, "matches", qReturnArg(ret), word);
        return ret;
    }

}
