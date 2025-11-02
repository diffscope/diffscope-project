#ifndef DIFFSCOPE_COREPLUGIN_QUICKINPUT_H
#define DIFFSCOPE_COREPLUGIN_QUICKINPUT_H

#include <qqmlintegration.h>

#include <QObject>
#include <QVariant>

#include <SVSCraftCore/SVSCraftNamespace.h>

#include <coreplugin/coreglobal.h>

namespace Core {

    class QuickInputPrivate;
    class WindowInterface;

    class CORE_EXPORT QuickInput : public QObject {
        Q_OBJECT
        QML_ELEMENT
        Q_DECLARE_PRIVATE(QuickInput)

        Q_PROPERTY(QString placeholderText READ placeholderText WRITE setPlaceholderText NOTIFY placeholderTextChanged)
        Q_PROPERTY(QString promptText READ promptText WRITE setPromptText NOTIFY promptTextChanged)
        Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged)
        Q_PROPERTY(SVS::SVSCraft::ControlType status READ status WRITE setStatus NOTIFY statusChanged)
        Q_PROPERTY(bool acceptable READ acceptable WRITE setAcceptable NOTIFY acceptableChanged)
        Q_PROPERTY(WindowInterface *windowHandle READ windowHandle WRITE setWindowHandle NOTIFY windowHandleChanged)
        Q_PROPERTY(bool visible READ visible WRITE setVisible NOTIFY visibleChanged)

    public:
        explicit QuickInput(QObject *parent = nullptr);
        ~QuickInput() override;

        QString placeholderText() const;
        void setPlaceholderText(const QString &placeholderText);

        QString promptText() const;
        void setPromptText(const QString &promptText);

        QString text() const;
        void setText(const QString &text);

        SVS::SVSCraft::ControlType status() const;
        void setStatus(SVS::SVSCraft::ControlType status);

        bool acceptable() const;
        void setAcceptable(bool acceptable);

        WindowInterface *windowHandle() const;
        void setWindowHandle(WindowInterface *windowHandle);

        bool visible() const;
        void setVisible(bool visible);

    public Q_SLOTS:
        void show();
        QVariant exec();
        void accept();
        void done(const QVariant &result);
        void reject();

    Q_SIGNALS:
        void placeholderTextChanged(const QString &placeholderText);
        void promptTextChanged(const QString &promptText);
        void textChanged(const QString &text);
        void statusChanged(SVS::SVSCraft::ControlType status);
        void acceptableChanged(bool acceptable);
        void windowHandleChanged(Core::WindowInterface *windowHandle);
        void visibleChanged(bool visible);

        void accepted();
        void rejected();
        void finished(const QVariant &result);
        void attemptingAcceptButFailed();

    private Q_SLOTS:
        void updateFromInputPalette();
        void handleWindowHandleDestroyed();

    private:
        QScopedPointer<QuickInputPrivate> d_ptr;
    };

}

#endif //DIFFSCOPE_COREPLUGIN_QUICKINPUT_H
