#ifndef DIFFSCOPE_COREPLUGIN_FILEBACKUPPAGE_H
#define DIFFSCOPE_COREPLUGIN_FILEBACKUPPAGE_H

#include <CoreApi/isettingpage.h>

namespace Core::Internal {

    class CorePlugin;

    class FileBackupPage : public ISettingPage {
        Q_OBJECT
    public:
        explicit FileBackupPage(QObject *parent = nullptr);
        ~FileBackupPage() override;

        bool matches(const QString &word) override;
        QString sortKeyword() const override;
        QObject *widget() override;
        void beginSetting() override;
        bool accept() override;
        void endSetting() override;

    private:
        friend class CorePlugin;
        bool widgetMatches(const QString &word);
        QObject *m_widget{};
    };

}

#endif //DIFFSCOPE_COREPLUGIN_FILEBACKUPPAGE_H