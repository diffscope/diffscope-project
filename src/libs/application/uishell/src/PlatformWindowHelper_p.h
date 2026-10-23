// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#ifndef DIFFSCOPE_UISHELL_PLATFORMWINDOWHELPER_P_H
#define DIFFSCOPE_UISHELL_PLATFORMWINDOWHELPER_P_H

#include <QObject>
#include <QString>
#include <QWindow>
#include <qqmlintegration.h>

namespace UIShell {

    class PlatformWindowHelper : public QObject {
        Q_OBJECT
        QML_ELEMENT

        Q_PROPERTY(QWindow *window READ window WRITE setWindow NOTIFY windowChanged)
        Q_PROPERTY(bool windowModified READ windowModified WRITE setWindowModified NOTIFY windowModifiedChanged)
        Q_PROPERTY(QString windowTitle READ windowTitle NOTIFY windowTitleChanged)

    public:
        explicit PlatformWindowHelper(QObject *parent = nullptr);
        ~PlatformWindowHelper() override;

        QWindow *window() const;
        void setWindow(QWindow *window);

        bool windowModified() const;
        void setWindowModified(bool modified);

        QString windowTitle() const;

    Q_SIGNALS:
        void windowChanged();
        void windowModifiedChanged();
        void windowTitleChanged();

    protected:
        bool eventFilter(QObject *watched, QEvent *event) override;

    private:
        void updatePlatformWindow();

        QWindow *m_window = nullptr;
        bool m_windowModified = false;
    };

}

#endif // DIFFSCOPE_UISHELL_PLATFORMWINDOWHELPER_P_H
