#include "audiooutputpage.h"

#include <QApplication>
#include <QQmlComponent>

#include <CoreApi/plugindatabase.h>

namespace Audio::Internal {

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
        QQmlComponent component(Core::PluginDatabase::qmlEngine(), "DiffScope.Audio", "AudioOutputPage");
        if (component.isError()) {
            qFatal() << component.errorString();
        }
        m_widget = component.createWithInitialProperties({{"pageHandle", QVariant::fromValue(this)}});
        m_widget->setParent(this);
        return m_widget;
    }

    void AudioOutputPage::beginSetting() {
        widget();
        m_widget->setProperty("started", true);
        ISettingPage::beginSetting();
    }

    bool AudioOutputPage::accept() {
        return Core::ISettingPage::accept();
    }

    void AudioOutputPage::endSetting() {
        Core::ISettingPage::endSetting();
    }

    bool AudioOutputPage::widgetMatches(const QString &word) {
        widget();
        auto matcher = m_widget->property("matcher").value<QObject *>();
        bool ret = false;
        QMetaObject::invokeMethod(matcher, "matches", qReturnArg(ret), word);
        return ret;
    }

}