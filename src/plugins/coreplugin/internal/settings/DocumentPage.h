// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#ifndef DIFFSCOPE_COREPLUGIN_DOCUMENTPAGE_H
#define DIFFSCOPE_COREPLUGIN_DOCUMENTPAGE_H

#include <CoreApi/isettingpage.h>

Q_MOC_INCLUDE(<QQuickWindow>)

class QQuickWindow;

namespace Core {
    class SourcesPickerModel;
}

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

        Q_INVOKABLE void editDefaultSinger(QQuickWindow *window);

    private:
        bool widgetMatches(const QString &word);
        QObject *m_widget{};
        SourcesPickerModel *m_defaultSingerModel{};
    };

}

#endif // DIFFSCOPE_COREPLUGIN_DOCUMENTPAGE_H
