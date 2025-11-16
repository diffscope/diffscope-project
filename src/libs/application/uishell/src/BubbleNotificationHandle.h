#ifndef UISHELL_BUBBLENOTIFICATIONHANDLE_H
#define UISHELL_BUBBLENOTIFICATIONHANDLE_H

#include <QDateTime>
#include <QObject>

#include <SVSCraftCore/SVSCraftNamespace.h>

#include <uishell/UIShellGlobal.h>

namespace UIShell {

    class UISHELL_EXPORT BubbleNotificationHandle : public QObject {
        Q_OBJECT
        Q_PROPERTY(QString title READ title NOTIFY titleChanged)
        Q_PROPERTY(QString text READ text NOTIFY textChanged)
        Q_PROPERTY(SVS::SVSCraft::MessageBoxIcon icon READ icon NOTIFY iconChanged)
        Q_PROPERTY(QStringList buttons READ buttons NOTIFY buttonsChanged)
        Q_PROPERTY(int primaryButton READ primaryButton NOTIFY primaryButtonChanged)
        Q_PROPERTY(bool closable READ closable NOTIFY closableChanged)
        Q_PROPERTY(bool hasProgress READ hasProgress NOTIFY hasProgressChanged)
        Q_PROPERTY(double progress READ progress NOTIFY progressChanged)
        Q_PROPERTY(bool progressAbortable READ progressAbortable NOTIFY progressAbortableChanged)
        Q_PROPERTY(bool permanentlyHideable READ permanentlyHideable NOTIFY permanentlyHideableChanged)
        Q_PROPERTY(int textFormat READ textFormat WRITE setTextFormat NOTIFY textFormatChanged)
        Q_PROPERTY(QDateTime time READ time NOTIFY timeChanged)

    public:
        inline explicit BubbleNotificationHandle(QObject *parent = nullptr) : QObject(parent) {
        }
        inline ~BubbleNotificationHandle() override = default;

        inline QString title() const { return m_title; }
        inline void setTitle(const QString &title) {
            if (m_title != title) {
                m_title = title;
                emit titleChanged();
            }
        }

        inline QString text() const { return m_text; }
        inline void setText(const QString &text) {
            if (m_text != text) {
                m_text = text;
                emit textChanged();
            }
        }

        inline SVS::SVSCraft::MessageBoxIcon icon() const { return m_icon; }
        inline void setIcon(const SVS::SVSCraft::MessageBoxIcon &icon) {
            if (m_icon != icon) {
                m_icon = icon;
                emit iconChanged();
            }
        }

        inline QStringList buttons() const { return m_buttons; }
        inline void setButtons(const QStringList &buttons) {
            if (m_buttons != buttons) {
                m_buttons = buttons;
                emit buttonsChanged();
            }
        }

        inline int primaryButton() const { return m_primaryButton; }
        inline void setPrimaryButton(int primaryButton) {
            if (m_primaryButton != primaryButton) {
                m_primaryButton = primaryButton;
                emit primaryButtonChanged();
            }
        }

        inline bool closable() const { return m_closable; }
        inline void setClosable(bool closable) {
            if (m_closable != closable) {
                m_closable = closable;
                emit closableChanged();
            }
        }

        inline bool hasProgress() const { return m_hasProgress; }
        inline void setHasProgress(bool hasProgress) {
            if (m_hasProgress != hasProgress) {
                m_hasProgress = hasProgress;
                emit hasProgressChanged();
            }
        }

        inline double progress() const { return m_progress; }
        inline void setProgress(double progress) {
            if (!qFuzzyCompare(m_progress, progress)) {
                m_progress = progress;
                emit progressChanged();
            }
        }

        inline bool progressAbortable() const { return m_progressAbortable; }
        inline void setProgressAbortable(bool progressAbortable) {
            if (m_progressAbortable != progressAbortable) {
                m_progressAbortable = progressAbortable;
                emit progressAbortableChanged();
            }
        }

        inline bool permanentlyHideable() const { return m_permanentlyHideable; }
        inline void setPermanentlyHideable(bool permanentlyHideable) {
            if (m_permanentlyHideable != permanentlyHideable) {
                m_permanentlyHideable = permanentlyHideable;
                emit permanentlyHideableChanged();
            }
        }

        enum TextFormat {
            StyledText = 4,
        };
        inline int textFormat() const { return m_textFormat; }
        inline void setTextFormat(int textFormat) {
            if (m_textFormat != textFormat) {
                m_textFormat = textFormat;
                emit textFormatChanged();
            }
        }

        inline QDateTime time() const { return m_time; }
        inline void setTime(const QDateTime &time) {
            if (m_time != time) {
                m_time = time;
                emit timeChanged();
            }
        }

    signals:
        void titleChanged();
        void textChanged();
        void iconChanged();
        void buttonsChanged();
        void primaryButtonChanged();
        void closableChanged();
        void hasProgressChanged();
        void progressChanged();
        void progressAbortableChanged();
        void permanentlyHideableChanged();
        void textFormatChanged();
        void timeChanged();

        void hideClicked();
        void closeClicked();
        void abortClicked();
        void permanentlyHideClicked();
        void buttonClicked(int index);
        void hoverEntered();
        void hoverExited();
        void linkActivated(const QString &link);

    private:
        QString m_title;
        QString m_text;
        SVS::SVSCraft::MessageBoxIcon m_icon{};
        QStringList m_buttons;
        int m_primaryButton{};
        bool m_closable{};
        bool m_hasProgress{};
        double m_progress{};
        bool m_progressAbortable{};
        bool m_permanentlyHideable{};
        int m_textFormat{Qt::AutoText};
        QDateTime m_time{};
    };

}

#endif //UISHELL_BUBBLENOTIFICATIONHANDLE_H
