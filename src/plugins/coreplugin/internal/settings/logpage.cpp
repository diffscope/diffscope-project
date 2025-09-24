#include "logpage.h"

#include <QQmlComponent>
#include <QQuickItem>
#include <QLoggingCategory>

#include <CoreApi/runtimeinterface.h>
#include <CoreApi/logger.h>

namespace Core::Internal {

    Q_STATIC_LOGGING_CATEGORY(lcLogPage, "diffscope.core.logpage")

    LogPage::LogPage(QObject *parent) : ISettingPage("core.Log", parent) {
        setTitle(tr("Log"));
        setDescription(tr("Configure log output and archiving behaviors"));
    }

    LogPage::~LogPage() {
        delete m_widget;
    }

    bool LogPage::matches(const QString &word) {
        return ISettingPage::matches(word) || widgetMatches(word);
    }

    QString LogPage::sortKeyword() const {
        return QStringLiteral("Log");
    }

    QObject *LogPage::widget() {
        if (m_widget)
            return m_widget;
        qCDebug(lcLogPage) << "Creating widget";
        QQmlComponent component(RuntimeInterface::qmlEngine(), "DiffScope.Core", "LogPage");
        if (component.isError()) {
            qFatal() << component.errorString();
        }
        m_widget = component.createWithInitialProperties({{"pageHandle", QVariant::fromValue(this)}});
        m_widget->setParent(this);
        return m_widget;
    }

    void LogPage::beginSetting() {
        qCInfo(lcLogPage) << "Beginning setting";
        widget();
        auto logger = RuntimeInterface::logger();
        m_widget->setProperty("maxFileSize", logger->maxFileSize());
        qCDebug(lcLogPage) << "maxFileSize" << m_widget->property("maxFileSize");
        m_widget->setProperty("maxArchiveSize", logger->maxArchiveSize());
        qCDebug(lcLogPage) << "maxArchiveSize" << m_widget->property("maxArchiveSize");
        m_widget->setProperty("maxArchiveDays", logger->maxArchiveDays());
        qCDebug(lcLogPage) << "maxArchiveDays" << m_widget->property("maxArchiveDays");
        m_widget->setProperty("prettifiesConsoleOutput", logger->prettifiesConsoleOutput());
        qCDebug(lcLogPage) << "prettifiesConsoleOutput" << m_widget->property("prettifiesConsoleOutput");
        m_widget->setProperty("consoleLogLevel", static_cast<int>(logger->consoleLogLevel()));
        qCDebug(lcLogPage) << "consoleLogLevel" << m_widget->property("consoleLogLevel");
        m_widget->setProperty("fileLogLevel", static_cast<int>(logger->fileLogLevel()));
        qCDebug(lcLogPage) << "fileLogLevel" << m_widget->property("fileLogLevel");
        m_widget->setProperty("started", true);
        ISettingPage::beginSetting();
    }

    bool LogPage::accept() {
        qCInfo(lcLogPage) << "Accepting";
        auto logger = RuntimeInterface::logger();
        qCDebug(lcLogPage) << "maxFileSize" << m_widget->property("maxFileSize");
        logger->setMaxFileSize(m_widget->property("maxFileSize").value<qsizetype>());
        qCDebug(lcLogPage) << "maxArchiveSize" << m_widget->property("maxArchiveSize");
        logger->setMaxArchiveSize(m_widget->property("maxArchiveSize").value<qsizetype>());
        qCDebug(lcLogPage) << "maxArchiveDays" << m_widget->property("maxArchiveDays");
        logger->setMaxArchiveDays(m_widget->property("maxArchiveDays").toInt());
        qCDebug(lcLogPage) << "prettifiesConsoleOutput" << m_widget->property("prettifiesConsoleOutput");
        logger->setPrettifiesConsoleOutput(m_widget->property("prettifiesConsoleOutput").toBool());
        qCDebug(lcLogPage) << "consoleLogLevel" << m_widget->property("consoleLogLevel");
        logger->setConsoleLogLevel(static_cast<Core::Logger::MessageType>(m_widget->property("consoleLogLevel").toInt()));
        qCDebug(lcLogPage) << "fileLogLevel" << m_widget->property("fileLogLevel");
        logger->setFileLogLevel(static_cast<Core::Logger::MessageType>(m_widget->property("fileLogLevel").toInt()));
        logger->saveSettings();
        return ISettingPage::accept();
    }

    void LogPage::endSetting() {
        qCInfo(lcLogPage) << "Ending setting";
        m_widget->setProperty("started", false);
        ISettingPage::endSetting();
    }

    bool LogPage::widgetMatches(const QString &word) {
        widget();
        auto matcher = m_widget->property("matcher").value<QObject *>();
        bool ret = false;
        QMetaObject::invokeMethod(matcher, "matches", qReturnArg(ret), word);
        return ret;
    }

}