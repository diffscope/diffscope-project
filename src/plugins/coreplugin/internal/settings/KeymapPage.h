#ifndef DIFFSCOPE_COREPLUGIN_KEYMAPPAGE_H
#define DIFFSCOPE_COREPLUGIN_KEYMAPPAGE_H

#include <CoreApi/isettingpage.h>

namespace Core::Internal {
    class KeyMapPage : public ISettingPage {
        Q_OBJECT
    public:
        explicit KeyMapPage(QObject *parent = nullptr);
        ~KeyMapPage() override;

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

#endif //DIFFSCOPE_COREPLUGIN_KEYMAPPAGE_H
