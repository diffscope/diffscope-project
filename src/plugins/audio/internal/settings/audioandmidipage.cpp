#include "audioandmidipage.h"

#include <QApplication>

namespace Audio::Internal {

    AudioAndMidiPage::AudioAndMidiPage(QObject *parent) : Core::ISettingPage("audio.AudioAndMidi", parent) {
        setTitle(tr("Audio and MIDI"));
        setDescription(tr("Configure audio and MIDI preferences"));
    }

    AudioAndMidiPage::~AudioAndMidiPage() = default;

    bool AudioAndMidiPage::matches(const QString &word) {
        return Core::ISettingPage::matches(word);
    }

    QString AudioAndMidiPage::sortKeyword() const {
        return QStringLiteral("AudioAndMidi");
    }

    QObject *AudioAndMidiPage::widget() {
        return nullptr;
    }

    void AudioAndMidiPage::beginSetting() {
        Core::ISettingPage::beginSetting();
    }

    bool AudioAndMidiPage::accept() {
        return Core::ISettingPage::accept();
    }

    void AudioAndMidiPage::endSetting() {
        Core::ISettingPage::endSetting();
    }

}