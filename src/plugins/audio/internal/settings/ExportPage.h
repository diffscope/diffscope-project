#ifndef DIFFSCOPE_AUDIOPLUGIN_EXPORTPAGE_H
#define DIFFSCOPE_AUDIOPLUGIN_EXPORTPAGE_H

#include <CoreApi/isettingpage.h>

namespace Audio::Internal {

    class ExportPage : public Core::ISettingPage {
        Q_OBJECT
    public:
        explicit ExportPage(QObject *parent = nullptr);
        ~ExportPage() override;

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

#endif // DIFFSCOPE_AUDIOPLUGIN_EXPORTPAGE_H
