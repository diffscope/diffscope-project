#include "FileBackupPage.h"

#include <QApplication>
#include <QLoggingCategory>
#include <QQmlComponent>
#include <QQuickItem>

#include <CoreApi/runtimeinterface.h>

#include <coreplugin/internal/BehaviorPreference.h>

namespace Core::Internal {

    Q_STATIC_LOGGING_CATEGORY(lcFileBackupPage, "diffscope.core.filebackuppage")

    FileBackupPage::FileBackupPage(QObject *parent) : ISettingPage("core.FileBackup", parent) {
        setTitle(tr("File and Backup"));
        setDescription(tr("Configure file handling and backup behaviors of %1").arg(QApplication::applicationDisplayName()));
    }

    FileBackupPage::~FileBackupPage() {
        delete m_widget;
    }

    bool FileBackupPage::matches(const QString &word) {
        return ISettingPage::matches(word) || widgetMatches(word);
    }

    QString FileBackupPage::sortKeyword() const {
        return QStringLiteral("FileBackup");
    }

    QObject *FileBackupPage::widget() {
        if (m_widget)
            return m_widget;
        qCDebug(lcFileBackupPage) << "Creating widget";
        QQmlComponent component(RuntimeInterface::qmlEngine(), "DiffScope.Core", "FileBackupPage");
        if (component.isError()) {
            qFatal() << component.errorString();
        }
        m_widget = component.createWithInitialProperties({{"pageHandle", QVariant::fromValue(this)}});
        m_widget->setParent(this);
        return m_widget;
    }

    void FileBackupPage::beginSetting() {
        qCInfo(lcFileBackupPage) << "Beginning setting";
        widget();
        m_widget->setProperty("fileOption", BehaviorPreference::instance()->property("fileOption"));
        qCDebug(lcFileBackupPage) << "fileOption" << m_widget->property("fileOption");
        m_widget->setProperty("started", true);
        ISettingPage::beginSetting();
    }

    bool FileBackupPage::accept() {
        qCInfo(lcFileBackupPage) << "Accepting";
        qCDebug(lcFileBackupPage) << "fileOption" << m_widget->property("fileOption");
        BehaviorPreference::instance()->setProperty("fileOption", m_widget->property("fileOption"));
        BehaviorPreference::instance()->save();
        return ISettingPage::accept();
    }

    void FileBackupPage::endSetting() {
        qCInfo(lcFileBackupPage) << "Ending setting";
        m_widget->setProperty("started", false);
        ISettingPage::endSetting();
    }

    bool FileBackupPage::widgetMatches(const QString &word) {
        widget();
        auto matcher = m_widget->property("matcher").value<QObject *>();
        bool ret = false;
        QMetaObject::invokeMethod(matcher, "matches", qReturnArg(ret), word);
        return ret;
    }

}
