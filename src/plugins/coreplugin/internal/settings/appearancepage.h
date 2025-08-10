#ifndef DIFFSCOPE_COREPLUGIN_APPEARANCEPAGE_H
#define DIFFSCOPE_COREPLUGIN_APPEARANCEPAGE_H

#include <CoreApi/isettingpage.h>

namespace Core::Internal {

    class AppearancePage : public ISettingPage {
        Q_OBJECT
    public:
        explicit AppearancePage(QObject *parent = nullptr);
        ~AppearancePage() override;

        QString sortKeyword() const override;
        bool matches(const QString &word) override;
        QObject *widget() override;
        void beginSetting() override;
        bool accept() override;
        void endSetting() override;

        Q_INVOKABLE static QStringList fontFamilies();
        Q_INVOKABLE static QStringList fontStyles(const QString &family);

    private:
        bool widgetMatches(const QString &word);
        mutable QObject *m_widget{};
    };

}

#endif //DIFFSCOPE_COREPLUGIN_APPEARANCEPAGE_H
