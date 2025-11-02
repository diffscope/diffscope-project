#ifndef DIFFSCOPE_COREPLUGIN_GENERALPAGE_H
#define DIFFSCOPE_COREPLUGIN_GENERALPAGE_H

#include <CoreApi/isettingpage.h>

namespace Core::Internal {

    class CorePlugin;

    class GeneralPage : public ISettingPage {
        Q_OBJECT
        Q_PROPERTY(QVariantList languages READ languages CONSTANT)
    public:
        explicit GeneralPage(QObject *parent = nullptr);
        ~GeneralPage() override;

        bool matches(const QString &word) override;
        QString sortKeyword() const override;
        QObject *widget() override;
        void beginSetting() override;
        bool accept() override;
        void endSetting() override;

        static QVariantList languages();

    private:
        friend class CorePlugin;
        bool widgetMatches(const QString &word);
        static QPair<QString, QString> getRestartMessageInNewLanguage(const QString &localeName);
        static void setCorePluginTranslationsPath(const QString &path);
        QObject *m_widget{};
    };

}

#endif //DIFFSCOPE_COREPLUGIN_GENERALPAGE_H
