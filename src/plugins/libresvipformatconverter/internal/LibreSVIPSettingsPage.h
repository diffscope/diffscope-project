#ifndef DIFFSCOPE_LIBRESVIP_FORMAT_CONVERTER_LIBRESVIPSETTINGSPAGE_H
#define DIFFSCOPE_LIBRESVIP_FORMAT_CONVERTER_LIBRESVIPSETTINGSPAGE_H

#include <CoreApi/isettingpage.h>

class QWindow;

namespace LibreSVIPFormatConverter::Internal {

    class LibreSVIPSettingsPage : public Core::ISettingPage {
        Q_OBJECT
        Q_PROPERTY(QString executablePath READ executablePath NOTIFY executablePathChanged)
        Q_PROPERTY(bool downloadedInstallationExists READ downloadedInstallationExists NOTIFY downloadedInstallationExistsChanged)
    public:
        explicit LibreSVIPSettingsPage(QObject *parent = nullptr);
        ~LibreSVIPSettingsPage() override;

        QString executablePath() const;
        bool downloadedInstallationExists() const;

        QString sortKeyword() const override;
        bool matches(const QString &word) override;
        QObject *widget() override;

        Q_INVOKABLE void browse();
        Q_INVOKABLE void download();
        Q_INVOKABLE void removeDownloadedInstallation();

    Q_SIGNALS:
        void executablePathChanged();
        void downloadedInstallationExistsChanged();

    private:
        bool widgetMatches(const QString &word);
        QWindow *pageWindow() const;

        QObject *m_widget{};
    };

}

#endif // DIFFSCOPE_LIBRESVIP_FORMAT_CONVERTER_LIBRESVIPSETTINGSPAGE_H
