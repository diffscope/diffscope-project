#ifndef DIFFSCOPE_COREPLUGIN_TIMEINDICATORPAGE_H
#define DIFFSCOPE_COREPLUGIN_TIMEINDICATORPAGE_H

#include <CoreApi/isettingpage.h>

namespace Core::Internal {

    class TimeIndicatorPage : public ISettingPage {
        Q_OBJECT
    public:
        explicit TimeIndicatorPage(QObject *parent = nullptr);
        ~TimeIndicatorPage() override;

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

#endif //DIFFSCOPE_COREPLUGIN_TIMEINDICATORPAGE_H
