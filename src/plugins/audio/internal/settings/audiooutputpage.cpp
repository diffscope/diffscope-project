#include "audiooutputpage.h"

#include <QApplication>
#include <QLoggingCategory>
#include <QQmlComponent>

#include <CoreApi/runtimeinterface.h>

#include <audio/internal/audiooutputsettingshelper.h>

namespace Audio::Internal {

    Q_STATIC_LOGGING_CATEGORY(lcAudioOutputPage, "diffscope.audio.audiooutputpage")

    AudioOutputPage::AudioOutputPage(QObject *parent) : Core::ISettingPage("audio.AudioOutput", parent) {
        setTitle(tr("Audio Output"));
        setDescription(tr("Configure the audio driver and device for output"));
    }

    AudioOutputPage::~AudioOutputPage() = default;

    bool AudioOutputPage::matches(const QString &word) {
        return ISettingPage::matches(word) || widgetMatches(word);
    }

    QString AudioOutputPage::sortKeyword() const {
        return QStringLiteral("AudioOutput");
    }

    QObject *AudioOutputPage::widget() {
        if (m_widget)
            return m_widget;
        qCDebug(lcAudioOutputPage) << "Creating widget";
        QQmlComponent component(Core::RuntimeInterface::qmlEngine(), "DiffScope.Audio", "AudioOutputPage");
        if (component.isError()) {
            qFatal() << component.errorString();
        }
        m_widget = component.createWithInitialProperties({{"pageHandle", QVariant::fromValue(this)}});
        m_widget->setParent(this);
        return m_widget;
    }

    void AudioOutputPage::beginSetting() {
        qCInfo(lcAudioOutputPage) << "Beginning setting";
        widget();
        m_widget->setProperty("started", true);
        m_widget->setProperty("helper", QVariant::fromValue(new AudioOutputSettingsHelper(this)));
        ISettingPage::beginSetting();
    }

    bool AudioOutputPage::accept() {
        qCInfo(lcAudioOutputPage) << "Accepting";
        return ISettingPage::accept();
    }

    void AudioOutputPage::endSetting() {
        qCInfo(lcAudioOutputPage) << "Ending setting";
        m_widget->setProperty("started", false);
        auto o = m_widget->property("helper").value<QObject *>();
        o->deleteLater();
        ISettingPage::endSetting();
    }

    bool AudioOutputPage::widgetMatches(const QString &word) {
        widget();
        auto matcher = m_widget->property("matcher").value<QObject *>();
        bool ret = false;
        QMetaObject::invokeMethod(matcher, "matches", qReturnArg(ret), word);
        return ret;
    }

}
