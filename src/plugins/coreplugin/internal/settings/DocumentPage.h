// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#ifndef DIFFSCOPE_COREPLUGIN_DOCUMENTPAGE_H
#define DIFFSCOPE_COREPLUGIN_DOCUMENTPAGE_H

#include <CoreApi/isettingpage.h>

namespace Core::Internal {

    class DocumentPage : public ISettingPage {
        Q_OBJECT
    public:
        explicit DocumentPage(QObject *parent = nullptr);
        ~DocumentPage() override;

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

#endif // DIFFSCOPE_COREPLUGIN_DOCUMENTPAGE_H
