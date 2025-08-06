#ifndef DIFFSCOPE_COREPLUGIN_GENERALPAGE_H
#define DIFFSCOPE_COREPLUGIN_GENERALPAGE_H

#include <CoreApi/isettingpage.h>

namespace Core::Internal {

    class GeneralPage : public ISettingPage {
        Q_OBJECT
    public:
        explicit GeneralPage(QObject *parent = nullptr);
        ~GeneralPage() override;

        bool matches(const QString &word) const override;
        QString sortKeyword() const override;
        QObject *widget() override;
        void beginSetting() override;
        bool accept() override;
        void endSetting() override;

    private:
        QObject *m_widget{};

    };

}

#endif //DIFFSCOPE_COREPLUGIN_GENERALPAGE_H
