#ifndef DIFFSCOPE_AUDIOPLUGIN_AUDIOANDMIDIPAGE_H
#define DIFFSCOPE_AUDIOPLUGIN_AUDIOANDMIDIPAGE_H

#include <CoreApi/isettingpage.h>

namespace Audio::Internal {

    class AudioAndMidiPage : public Core::ISettingPage {
        Q_OBJECT
    public:
        explicit AudioAndMidiPage(QObject *parent = nullptr);
        ~AudioAndMidiPage() override;

        bool matches(const QString &word) override;
        QString sortKeyword() const override;
        QObject *widget() override;
        void beginSetting() override;
        bool accept() override;
        void endSetting() override;
    };

}

#endif //DIFFSCOPE_AUDIOPLUGIN_AUDIOANDMIDIPAGE_H
