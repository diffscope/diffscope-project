#ifndef DIFFSCOPE_COREPLUGIN_NOTIFICATIONMESSAGE_H
#define DIFFSCOPE_COREPLUGIN_NOTIFICATIONMESSAGE_H

#include <QObject>
#include <qqmlintegration.h>

#include <SVSCraftCore/SVSCraftNamespace.h>

#include <coreplugin/coreglobal.h>

namespace Core {

    class NotificationMessagePrivate;

    class CORE_EXPORT NotificationMessage : public QObject {
        Q_OBJECT
        QML_ELEMENT
        Q_DECLARE_PRIVATE(NotificationMessage)
        Q_PROPERTY(QString title READ title WRITE setTitle NOTIFY titleChanged)
        Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged)
        Q_PROPERTY(SVS::SVSCraft::MessageBoxIcon icon READ icon WRITE setIcon NOTIFY iconChanged)
        Q_PROPERTY(QStringList buttons READ buttons WRITE setButtons NOTIFY buttonsChanged)
        Q_PROPERTY(int primaryButton READ primaryButton WRITE setPrimaryButton NOTIFY primaryButtonChanged)
        Q_PROPERTY(bool closable READ closable WRITE setClosable NOTIFY closableChanged)
        Q_PROPERTY(bool hasProgress READ hasProgress WRITE setHasProgress NOTIFY hasProgressChanged)
        Q_PROPERTY(double progress READ progress WRITE setProgress NOTIFY progressChanged)
        Q_PROPERTY(bool progressAbortable READ progressAbortable WRITE setProgressAbortable NOTIFY progressAbortableChanged)
        Q_PROPERTY(bool allowDoNotShowAgain READ allowDoNotShowAgain WRITE setAllowDoNotShowAgain NOTIFY allowDoNotShowAgainChanged)
        Q_PROPERTY(int textFormat READ textFormat WRITE setTextFormat NOTIFY textFormatChanged)
        Q_PRIVATE_PROPERTY(NotificationMessage::d_func(), QObject *handle MEMBER handle CONSTANT)

    public:
        explicit NotificationMessage(QObject *parent = nullptr);
        ~NotificationMessage() override;

        QString title() const;
        void setTitle(const QString &title);

        QString text() const;
        void setText(const QString &text);

        SVS::SVSCraft::MessageBoxIcon icon() const;
        void setIcon(SVS::SVSCraft::MessageBoxIcon icon);

        QStringList buttons() const;
        void setButtons(const QStringList &buttons);

        int primaryButton() const;
        void setPrimaryButton(int primaryButton);

        bool closable() const;
        void setClosable(bool closable);

        bool hasProgress() const;
        void setHasProgress(bool hasProgress);

        double progress() const;
        void setProgress(double progress);

        bool progressAbortable() const;
        void setProgressAbortable(bool progressAbortable);

        bool allowDoNotShowAgain() const;
        void setAllowDoNotShowAgain(bool allow);

        int textFormat() const;
        void setTextFormat(int format);

        Q_INVOKABLE void hide();
        Q_INVOKABLE void close();

    signals:
        void titleChanged(const QString &title);
        void textChanged(const QString &text);
        void iconChanged(SVS::SVSCraft::MessageBoxIcon icon);
        void buttonsChanged(const QStringList &buttons);
        void primaryButtonChanged(int primaryButton);
        void closableChanged(bool closable);
        void hasProgressChanged(bool hasProgress);
        void progressChanged(double progress);
        void progressAbortableChanged(bool progressAbortable);
        void allowDoNotShowAgainChanged(bool allow);
        void textFormatChanged(int format);

        void hidden();
        void closed();
        void progressAborted();
        void doNotShowAgainRequested();
        void buttonClicked(int index);
        void linkActivated(const QString &link);

    private:
        QScopedPointer<NotificationMessagePrivate> d_ptr;
        
    };

}

#endif //DIFFSCOPE_COREPLUGIN_NOTIFICATIONMESSAGE_H
