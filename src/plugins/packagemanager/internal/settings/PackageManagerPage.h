#ifndef DIFFSCOPE_PACKAGEMANAGER_PACKAGEMANAGERPAGE_H
#define DIFFSCOPE_PACKAGEMANAGER_PACKAGEMANAGERPAGE_H

#include <CoreApi/isettingpage.h>

#include <QObject>
#include <QUrl>

namespace PackageManager {

    class PackageManagerPage : public Core::ISettingPage {
        Q_OBJECT
    public:
        explicit PackageManagerPage(QObject *parent = nullptr);
        ~PackageManagerPage() override;

        bool matches(const QString &word) override;
        QString sortKeyword() const override;
        QObject *widget() override;
        void beginSetting() override;
        bool accept() override;
        void endSetting() override;

        Q_INVOKABLE bool validateDspmPath(const QUrl &url, int timeoutSeconds);
        Q_INVOKABLE QString localFilePath(const QUrl &url);

    private:
        bool widgetMatches(const QString &word);

        QObject *m_widget{};
    };

}

#endif // DIFFSCOPE_PACKAGEMANAGER_PACKAGEMANAGERPAGE_H
