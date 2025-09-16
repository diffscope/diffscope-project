#ifndef DIFFSCOPE_COREPLUGIN_MENUPAGE_H
#define DIFFSCOPE_COREPLUGIN_MENUPAGE_H

#include <CoreApi/isettingpage.h>

namespace QAK {
    class ActionLayoutsModel;
}

namespace Core::Internal {
    class MenuPage : public ISettingPage {
        Q_OBJECT
    public:
        explicit MenuPage(QObject *parent = nullptr);
        ~MenuPage() override;

        bool matches(const QString &word) override;
        QString sortKeyword() const override;
        QObject *widget() override;
        void beginSetting() override;
        bool accept() override;
        void endSetting() override;

    private:
        bool widgetMatches(const QString &word);
        QObject *m_widget{};
        QAK::ActionLayoutsModel *m_actionLayoutsModel{};

    };
}

#endif //DIFFSCOPE_COREPLUGIN_MENUPAGE_H
