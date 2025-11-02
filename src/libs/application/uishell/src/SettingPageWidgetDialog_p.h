#ifndef UISHELL_SETTINGPAGEWIDGETDIALOG_P_H
#define UISHELL_SETTINGPAGEWIDGETDIALOG_P_H

#include <qqmlintegration.h>

#include <QDialog>
#include <QWindow>

namespace UIShell {

    class SettingPageWidgetDialog : public QDialog {
        Q_OBJECT
        QML_ELEMENT
        Q_PROPERTY(QWidget *widget READ widget WRITE setWidget NOTIFY widgetChanged)
        Q_PROPERTY(QWindow *windowHandle READ windowHandle CONSTANT)
    public:
        explicit SettingPageWidgetDialog(QWidget *parent = nullptr);
        ~SettingPageWidgetDialog() override;

        QWidget *widget() const;
        void setWidget(QWidget *widget);

        Q_INVOKABLE static inline bool isWidget(QObject *object) {
            return object->isWidgetType();
        }

    signals:
        void widgetChanged();

    private:
        QWidget *m_widget{};
    };

}

#endif //UISHELL_SETTINGPAGEWIDGETDIALOG_P_H
