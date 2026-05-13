#include "PlaybackPage.h"

#include <QLoggingCategory>
#include <QQmlComponent>

#include <CoreApi/runtimeinterface.h>

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

    bool PlaybackPage::widgetMatches(const QString &word) {
        widget();
        auto matcher = m_widget->property("matcher").value<QObject *>();
        bool ret = false;
        QMetaObject::invokeMethod(matcher, "matches", qReturnArg(ret), word);
        return ret;
    }

}
