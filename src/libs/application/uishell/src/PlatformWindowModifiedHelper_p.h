#ifndef UISHELL_PLATFORMWINDOWMODIFIEDHELPER_P_H
#define UISHELL_PLATFORMWINDOWMODIFIEDHELPER_P_H

#include <QObject>
#include <QWindow>
#include <qqmlintegration.h>

namespace UIShell {

    class PlatformWindowModifiedHelper : public QObject {
        Q_OBJECT
        QML_ELEMENT

        Q_PROPERTY(QWindow *window READ window WRITE setWindow NOTIFY windowChanged)
        Q_PROPERTY(bool windowModified READ windowModified WRITE setWindowModified NOTIFY windowModifiedChanged)

    public:
        explicit PlatformWindowModifiedHelper(QObject *parent = nullptr);
        ~PlatformWindowModifiedHelper() override;

        QWindow *window() const;
        void setWindow(QWindow *window);

        bool windowModified() const;
        void setWindowModified(bool modified);

    Q_SIGNALS:
        void windowChanged();
        void windowModifiedChanged();

    private:
        void updatePlatformWindowModified();

        QWindow *m_window = nullptr;
        bool m_windowModified = false;
    };

}

#endif // UISHELL_PLATFORMWINDOWMODIFIEDHELPER_P_H
