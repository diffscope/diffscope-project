#include "ExportPage.h"

#include <QLoggingCategory>
#include <QQmlComponent>

#include <CoreApi/runtimeinterface.h>

#include <audio/internal/AudioPreference.h>

namespace Audio::Internal {

    Q_STATIC_LOGGING_CATEGORY(lcExportPage, "diffscope.audio.exportpage")

    ExportPage::ExportPage(QObject *parent) : Core::ISettingPage("audio.Export", parent) {
        setTitle(tr("Export"));
        setDescription(tr("Configure audio export behavior"));
    }

    ExportPage::~ExportPage() {
        delete m_widget;
    }

    bool ExportPage::matches(const QString &word) {
        return ISettingPage::matches(word) || widgetMatches(word);
    }

    QString ExportPage::sortKeyword() const {
        return QStringLiteral("Export");
    }

    QObject *ExportPage::widget() {
        if (m_widget)
            return m_widget;
        qCDebug(lcExportPage) << "Creating widget";
        QQmlComponent component(Core::RuntimeInterface::qmlEngine(), "DiffScope.Audio", "ExportPage");
        if (component.isError()) {
            qFatal() << component.errorString();
        }
        m_widget = component.createWithInitialProperties({{"pageHandle", QVariant::fromValue(this)}});
        m_widget->setParent(this);
        return m_widget;
    }

    void ExportPage::beginSetting() {
        qCInfo(lcExportPage) << "Beginning setting";
        widget();
        m_widget->setProperty("audioExporterClippingCheckEnabled", AudioPreference::instance()->property("audioExporterClippingCheckEnabled"));
        qCDebug(lcExportPage) << "audioExporterClippingCheckEnabled" << m_widget->property("audioExporterClippingCheckEnabled");
        m_widget->setProperty("audioExporterUseTemporaryFile", AudioPreference::instance()->property("audioExporterUseTemporaryFile"));
        qCDebug(lcExportPage) << "audioExporterUseTemporaryFile" << m_widget->property("audioExporterUseTemporaryFile");
        m_widget->setProperty("shouldPlayNotificationSoundWhenExportCompleted", AudioPreference::instance()->property("shouldPlayNotificationSoundWhenExportCompleted"));
        qCDebug(lcExportPage) << "shouldPlayNotificationSoundWhenExportCompleted" << m_widget->property("shouldPlayNotificationSoundWhenExportCompleted");
        m_widget->setProperty("started", true);
        ISettingPage::beginSetting();
    }

    bool ExportPage::accept() {
        qCInfo(lcExportPage) << "Accepting";
        qCDebug(lcExportPage) << "audioExporterClippingCheckEnabled" << m_widget->property("audioExporterClippingCheckEnabled");
        AudioPreference::instance()->setProperty("audioExporterClippingCheckEnabled", m_widget->property("audioExporterClippingCheckEnabled"));
        qCDebug(lcExportPage) << "audioExporterUseTemporaryFile" << m_widget->property("audioExporterUseTemporaryFile");
        AudioPreference::instance()->setProperty("audioExporterUseTemporaryFile", m_widget->property("audioExporterUseTemporaryFile"));
        qCDebug(lcExportPage) << "shouldPlayNotificationSoundWhenExportCompleted" << m_widget->property("shouldPlayNotificationSoundWhenExportCompleted");
        AudioPreference::instance()->setProperty("shouldPlayNotificationSoundWhenExportCompleted", m_widget->property("shouldPlayNotificationSoundWhenExportCompleted"));
        AudioPreference::instance()->save();
        return ISettingPage::accept();
    }

    void ExportPage::endSetting() {
        qCInfo(lcExportPage) << "Ending setting";
        m_widget->setProperty("started", false);
        ISettingPage::endSetting();
    }

    bool ExportPage::widgetMatches(const QString &word) {
        widget();
        auto matcher = m_widget->property("matcher").value<QObject *>();
        bool ret = false;
        QMetaObject::invokeMethod(matcher, "matches", qReturnArg(ret), word);
        return ret;
    }

}
