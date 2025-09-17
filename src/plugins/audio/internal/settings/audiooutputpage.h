#ifndef DIFFSCOPE_AUDIOPLUGIN_AUDIOOUTPUTPAGE_H
#define DIFFSCOPE_AUDIOPLUGIN_AUDIOOUTPUTPAGE_H

#include <CoreApi/isettingpage.h>

namespace Audio::Internal {

    class AudioOutputPage : public Core::ISettingPage {
        Q_OBJECT
    public:
        explicit AudioOutputPage(QObject *parent = nullptr);
        ~AudioOutputPage() override;

        bool matches(const QString &word) override;
        QString sortKeyword() const override;
        QObject *widget() override;
        void beginSetting() override;
        bool accept() override;
        void endSetting() override;

    private:
        bool widgetMatches(const QString &word);
        QObject *m_widget{};

    };

}

#endif //DIFFSCOPE_AUDIOPLUGIN_AUDIOOUTPUTPAGE_H