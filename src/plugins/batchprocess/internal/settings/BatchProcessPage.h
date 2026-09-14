// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_BATCHPROCESS_BATCHPROCESSPAGE_H
#define DIFFSCOPE_BATCHPROCESS_BATCHPROCESSPAGE_H

#include <QUrl>

#include <CoreApi/isettingpage.h>

namespace BatchProcess::Internal {

    class BatchProcessPage : public Core::ISettingPage {
        Q_OBJECT
    public:
        explicit BatchProcessPage(QObject *parent = nullptr);
        ~BatchProcessPage() override;

        bool matches(const QString &word) override;
        QString sortKeyword() const override;
        QObject *widget() override;
        void beginSetting() override;
        bool accept() override;
        void endSetting() override;

        Q_INVOKABLE QString defaultScriptDirectory() const;
        Q_INVOKABLE QString defaultScriptDataDirectory() const;
        Q_INVOKABLE QString localFilePath(const QUrl &url) const;

    private:
        bool widgetMatches(const QString &word);

        QObject *m_widget{};
        QString m_initialScriptDirectory;
    };

}

#endif // DIFFSCOPE_BATCHPROCESS_BATCHPROCESSPAGE_H
