#include "logpage.h"

#include <QQmlComponent>
#include <QQuickItem>

#include <CoreApi/runtimeinterface.h>
#include <CoreApi/logger.h>

namespace Core::Internal {

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
        QQmlComponent component(RuntimeInterface::qmlEngine(), "DiffScope.Core", "LogPage");
        if (component.isError()) {
            qFatal() << component.errorString();
        }
        m_widget = component.createWithInitialProperties({{"pageHandle", QVariant::fromValue(this)}});
        m_widget->setParent(this);
        return m_widget;
    }

    void LogPage::beginSetting() {
        widget();
        auto logger = RuntimeInterface::logger();
        m_widget->setProperty("maxFileSize", logger->maxFileSize());
        m_widget->setProperty("maxArchiveSize", logger->maxArchiveSize());
        m_widget->setProperty("maxArchiveDays", logger->maxArchiveDays());
        m_widget->setProperty("prettifiesConsoleOutput", logger->prettifiesConsoleOutput());
        m_widget->setProperty("consoleLogLevel", static_cast<int>(logger->consoleLogLevel()));
        m_widget->setProperty("fileLogLevel", static_cast<int>(logger->fileLogLevel()));
        m_widget->setProperty("started", true);
        ISettingPage::beginSetting();
    }

    bool LogPage::accept() {
        auto logger = RuntimeInterface::logger();
        logger->setMaxFileSize(m_widget->property("maxFileSize").value<qsizetype>());
        logger->setMaxArchiveSize(m_widget->property("maxArchiveSize").value<qsizetype>());
        logger->setMaxArchiveDays(m_widget->property("maxArchiveDays").toInt());
        logger->setPrettifiesConsoleOutput(m_widget->property("prettifiesConsoleOutput").toBool());
        logger->setConsoleLogLevel(static_cast<Core::Logger::MessageType>(m_widget->property("consoleLogLevel").toInt()));
        logger->setFileLogLevel(static_cast<Core::Logger::MessageType>(m_widget->property("fileLogLevel").toInt()));
        logger->saveSettings();
        return ISettingPage::accept();
    }

    void LogPage::endSetting() {
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