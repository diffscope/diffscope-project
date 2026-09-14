// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "BatchProcessPage.h"

#include <QLoggingCategory>
#include <QQmlComponent>

#include <CoreApi/runtimeinterface.h>

#include <batchprocess/BatchProcessInterface.h>
#include <batchprocess/internal/BatchProcessSettings.h>

namespace BatchProcess::Internal {

    Q_STATIC_LOGGING_CATEGORY(lcBatchProcessPage, "diffscope.batchprocess.settings")

    BatchProcessPage::BatchProcessPage(QObject *parent)
        : Core::ISettingPage("org.diffscope.batchprocess.BatchProcess", parent) {
        setTitle(tr("Batch Process"));
        setDescription(tr("Configure batch processing scripts, file-system access, and the JavaScript console"));
    }

    BatchProcessPage::~BatchProcessPage() {
        delete m_widget;
    }

    bool BatchProcessPage::matches(const QString &word) {
        return Core::ISettingPage::matches(word) || widgetMatches(word);
    }

    QString BatchProcessPage::sortKeyword() const {
        return QStringLiteral("Batch Process");
    }

    QObject *BatchProcessPage::widget() {
        if (m_widget) {
            return m_widget;
        }

        QQmlComponent component(Core::RuntimeInterface::qmlEngine(), "DiffScope.BatchProcess", "BatchProcessPage");
        if (component.isError()) {
            qFatal() << component.errorString();
        }
        m_widget = component.createWithInitialProperties({{"pageHandle", QVariant::fromValue(this)}});
        if (!m_widget) {
            qFatal() << component.errorString();
        }
        m_widget->setParent(this);
        return m_widget;
    }

    void BatchProcessPage::beginSetting() {
        widget();
        m_initialScriptDirectory = BatchProcessSettings::scriptDirectory();
        m_widget->setProperty("scriptDirectory", m_initialScriptDirectory);
        m_widget->setProperty("scriptDataDirectory", BatchProcessSettings::scriptDataDirectory());
        m_widget->setProperty("maximumConsoleMessageCount", BatchProcessSettings::maximumConsoleMessageCount());
        m_widget->setProperty("started", true);
        Core::ISettingPage::beginSetting();
    }

    bool BatchProcessPage::accept() {
        const auto scriptDirectory = m_widget->property("scriptDirectory").toString();
        BatchProcessSettings::setScriptDirectory(scriptDirectory);
        BatchProcessSettings::setScriptDataDirectory(m_widget->property("scriptDataDirectory").toString());
        BatchProcessSettings::setMaximumConsoleMessageCount(m_widget->property("maximumConsoleMessageCount").toInt());
        BatchProcessSettings::instance()->save();
        if (BatchProcessSettings::scriptDirectory() != m_initialScriptDirectory) {
            qCInfo(lcBatchProcessPage) << "Script directory changed; reloading scripts";
            BatchProcessInterface::instance()->reloadScripts();
        }
        return Core::ISettingPage::accept();
    }

    void BatchProcessPage::endSetting() {
        m_widget->setProperty("started", false);
        Core::ISettingPage::endSetting();
    }

    QString BatchProcessPage::defaultScriptDirectory() const {
        return BatchProcessSettings::defaultScriptDirectory();
    }

    QString BatchProcessPage::defaultScriptDataDirectory() const {
        return BatchProcessSettings::defaultScriptDataDirectory();
    }

    QString BatchProcessPage::localFilePath(const QUrl &url) const {
        return url.isLocalFile() ? url.toLocalFile() : url.toString(QUrl::PreferLocalFile);
    }

    bool BatchProcessPage::widgetMatches(const QString &word) {
        widget();
        auto matcher = m_widget->property("matcher").value<QObject *>();
        bool result = false;
        QMetaObject::invokeMethod(matcher, "matches", qReturnArg(result), word);
        return result;
    }

}

#include "moc_BatchProcessPage.cpp"
