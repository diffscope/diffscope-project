#include "UpdatePage.h"

#include <QApplication>
#include <QLoggingCategory>
#include <QQmlComponent>
#include <QQuickItem>

#include <CoreApi/runtimeinterface.h>

#include <maintenance/internal/ApplicationUpdateChecker.h>

namespace Maintenance {

    Q_STATIC_LOGGING_CATEGORY(lcUpdatePage, "diffscope.maintenance.updatepage")

    UpdatePage::UpdatePage(QObject *parent) : Core::ISettingPage("org.diffscope.maintenance.Update", parent) {
        setTitle(tr("Update"));
        setDescription(tr("Configure update settings for %1").arg(QApplication::applicationDisplayName()));
    }

    UpdatePage::~UpdatePage() {
        delete m_widget;
    }

    bool UpdatePage::matches(const QString &word) {
        return Core::ISettingPage::matches(word) || widgetMatches(word);
    }

    QString UpdatePage::sortKeyword() const {
        return QStringLiteral("Update");
    }

    QObject *UpdatePage::widget() {
        if (m_widget)
            return m_widget;

        qCDebug(lcUpdatePage) << "Creating widget";
        QQmlComponent component(Core::RuntimeInterface::qmlEngine(), "DiffScope.Maintenance", "UpdatePage");
        if (component.isError()) {
            qFatal() << component.errorString();
        }
        m_widget = component.createWithInitialProperties({{"pageHandle", QVariant::fromValue(this)}});
        m_widget->setParent(this);
        return m_widget;
    }

    void UpdatePage::beginSetting() {
        qCInfo(lcUpdatePage) << "Beginning setting";
        widget();

        // Load current settings from ApplicationUpdateChecker singleton and set to widget properties
        m_widget->setProperty("autoCheckForUpdates", ApplicationUpdateChecker::instance()->property("autoCheckForUpdates"));
        qCDebug(lcUpdatePage) << "autoCheckForUpdates" << m_widget->property("autoCheckForUpdates");

        m_widget->setProperty("updateOption", ApplicationUpdateChecker::instance()->property("updateOption"));
        qCDebug(lcUpdatePage) << "updateOption" << m_widget->property("updateOption");

        // Mark the widget as started to enable dirty tracking
        m_widget->setProperty("started", true);
        Core::ISettingPage::beginSetting();
    }

    bool UpdatePage::accept() {
        qCInfo(lcUpdatePage) << "Accepting";

        // Save widget property values back to ApplicationUpdateChecker singleton
        qCDebug(lcUpdatePage) << "autoCheckForUpdates" << m_widget->property("autoCheckForUpdates");
        ApplicationUpdateChecker::instance()->setProperty("autoCheckForUpdates", m_widget->property("autoCheckForUpdates"));

        qCDebug(lcUpdatePage) << "updateOption" << m_widget->property("updateOption");
        ApplicationUpdateChecker::instance()->setProperty("updateOption", m_widget->property("updateOption"));

        // Persist settings to storage
        ApplicationUpdateChecker::instance()->save();
        return Core::ISettingPage::accept();
    }

    void UpdatePage::endSetting() {
        qCInfo(lcUpdatePage) << "Ending setting";
        m_widget->setProperty("started", false);
        Core::ISettingPage::endSetting();
    }

    bool UpdatePage::widgetMatches(const QString &word) {
        widget();
        auto matcher = m_widget->property("matcher").value<QObject *>();
        bool ret = false;
        QMetaObject::invokeMethod(matcher, "matches", qReturnArg(ret), word);
        return ret;
    }

}
