#ifndef UISHELL_DIGITALCLOCKLABEL_P_H
#define UISHELL_DIGITALCLOCKLABEL_P_H

#include <qqmlintegration.h>

#include <QObject>

#include <QtQuick/private/qquicktext_p.h>

namespace UIShell {

    class DigitalClockLabel : public QQuickText {
        Q_OBJECT
        QML_ELEMENT

        Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged)
        Q_PROPERTY(bool fineTuneEnabled READ fineTuneEnabled WRITE setFineTuneEnabled NOTIFY fineTuneEnabledChanged)

    public:
        explicit DigitalClockLabel(QQuickItem *parent = nullptr);
        ~DigitalClockLabel() override;

        QString text() const;
        void setText(const QString &text);

        bool fineTuneEnabled() const;
        void setFineTuneEnabled(bool enabled);

    Q_SIGNALS:
        void textChanged();
        void fineTuneEnabledChanged();

    private:
        void updateDigitalFormat();

        bool m_fineTuneEnabled = true;
        QString m_fullText;
    };

}

#endif // UISHELL_DIGITALCLOCKLABEL_P_H
