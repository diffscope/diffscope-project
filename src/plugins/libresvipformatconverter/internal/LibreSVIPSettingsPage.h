// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: GPL-3.0-only

#ifndef DIFFSCOPE_LIBRESVIP_FORMAT_CONVERTER_LIBRESVIPSETTINGSPAGE_H
#define DIFFSCOPE_LIBRESVIP_FORMAT_CONVERTER_LIBRESVIPSETTINGSPAGE_H

#include <QUrl>
#include <QVariant>

#include <CoreApi/isettingpage.h>

class QWindow;

namespace LibreSVIPFormatConverter::Internal {

    class LibreSVIPSettingsPage : public Core::ISettingPage {
        Q_OBJECT
        Q_PROPERTY(QString executablePath READ executablePath NOTIFY executablePathChanged)
        Q_PROPERTY(QVariantList installations READ installations NOTIFY installationsChanged)
        Q_PROPERTY(QString installationId READ installationId NOTIFY executablePathChanged)
        Q_PROPERTY(bool selectedInstallationIsDownloaded READ selectedInstallationIsDownloaded NOTIFY executablePathChanged)
        Q_PROPERTY(QUrl homepageUrl READ homepageUrl CONSTANT)
    public:
        explicit LibreSVIPSettingsPage(QObject *parent = nullptr);
        ~LibreSVIPSettingsPage() override;

        QString executablePath() const;
        QVariantList installations() const;
        QString installationId() const;
        bool selectedInstallationIsDownloaded() const;
        QUrl homepageUrl() const;

        QString sortKeyword() const override;
        bool matches(const QString &word) override;
        QObject *widget() override;
        void beginSetting() override;
        bool accept() override;
        void endSetting() override;

        Q_INVOKABLE void browse();
        Q_INVOKABLE void download();
        Q_INVOKABLE bool selectInstallation(const QString &id);
        Q_INVOKABLE void removeLocalInstallation();
        Q_INVOKABLE void uninstallSelectedInstallation();

    Q_SIGNALS:
        void executablePathChanged();
        void installationsChanged();

    private:
        bool widgetMatches(const QString &word);
        QWindow *pageWindow() const;

        QObject *m_widget{};
    };

}

#endif // DIFFSCOPE_LIBRESVIP_FORMAT_CONVERTER_LIBRESVIPSETTINGSPAGE_H
