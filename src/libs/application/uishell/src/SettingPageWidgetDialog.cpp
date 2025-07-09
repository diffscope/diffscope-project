#include "SettingPageWidgetDialog_p.h"

#include <QBoxLayout>
#include <QQuickWindow>
#include <QTimer>

namespace UIShell {

    SettingPageWidgetDialog::SettingPageWidgetDialog(QWidget *parent) : QDialog(parent) {
        setWindowFlags(Qt::Tool | Qt::FramelessWindowHint);
        setLayout(new QVBoxLayout);
        createWinId();
    }
    SettingPageWidgetDialog::~SettingPageWidgetDialog() = default;
    QWidget *SettingPageWidgetDialog::widget() const {
        return m_widget;
    }
    void SettingPageWidgetDialog::setWidget(QWidget *widget) {
        if (m_widget == widget)
            return;
        if (m_widget) {
            m_widget->setParent(nullptr);
        }
        m_widget = widget;
        if (m_widget) {
            m_widget->setParent(this);
        }
        emit widgetChanged();
    }


}