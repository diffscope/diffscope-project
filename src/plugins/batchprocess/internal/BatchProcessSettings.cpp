// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "BatchProcessSettings.h"

#include <algorithm>

#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <QLoggingCategory>
#include <QSettings>
#include <QStandardPaths>

#include <CoreApi/runtimeinterface.h>

#include <batchprocess/JavaScriptConsoleInterface.h>

namespace BatchProcess::Internal {

    Q_STATIC_LOGGING_CATEGORY(lcBatchProcessSettings, "diffscope.batchprocess.settings")

    static BatchProcessSettings *m_instance = nullptr;

    namespace {

        constexpr Qt::CaseSensitivity pathCaseSensitivity() {
#if defined(Q_OS_WIN)
            return Qt::CaseInsensitive;
#else
            return Qt::CaseSensitive;
#endif
        }

        QString normalizedPath(const QString &path) {
            if (path.trimmed().isEmpty()) {
                return {};
            }
            return QDir::toNativeSeparators(QDir::cleanPath(QFileInfo(path.trimmed()).absoluteFilePath()));
        }

        int boundedMaximumConsoleMessageCount(int maximumMessageCount) {
            return std::clamp(maximumMessageCount, BatchProcessSettings::MinimumConsoleMessageCount, BatchProcessSettings::MaximumConsoleMessageCount);
        }

    }

    BatchProcessSettings::BatchProcessSettings(QObject *parent) : QObject(parent) {
        Q_ASSERT(!m_instance);
        m_instance = this;
        load();
    }

    BatchProcessSettings::~BatchProcessSettings() {
        save();
        m_instance = nullptr;
    }

    BatchProcessSettings *BatchProcessSettings::instance() {
        return m_instance;
    }

    QString BatchProcessSettings::scriptDirectory() {
        Q_ASSERT(m_instance);
        return m_instance->m_customScriptDirectory.isEmpty() ? defaultScriptDirectory() : m_instance->m_customScriptDirectory;
    }

    void BatchProcessSettings::setScriptDirectory(const QString &directory) {
        Q_ASSERT(m_instance);
        const auto normalized = normalizedPath(directory);
        const auto normalizedDefault = normalizedPath(defaultScriptDirectory());
        m_instance->m_customScriptDirectory = normalized.compare(normalizedDefault, pathCaseSensitivity()) == 0 ? QString() : normalized;
    }

    QString BatchProcessSettings::defaultScriptDirectory() {
        auto documentsDirectory = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
        if (documentsDirectory.isEmpty()) {
            documentsDirectory = QDir::homePath();
        }
        return QDir::toNativeSeparators(QDir(documentsDirectory).filePath(QApplication::applicationName() + QStringLiteral("/Scripts")));
    }

    QString BatchProcessSettings::scriptDataDirectory() {
        Q_ASSERT(m_instance);
        return m_instance->m_customScriptDataDirectory.isEmpty() ? defaultScriptDataDirectory() : m_instance->m_customScriptDataDirectory;
    }

    void BatchProcessSettings::setScriptDataDirectory(const QString &directory) {
        Q_ASSERT(m_instance);
        const auto normalized = normalizedPath(directory);
        const auto normalizedDefault = normalizedPath(defaultScriptDataDirectory());
        m_instance->m_customScriptDataDirectory = normalized.compare(normalizedDefault, pathCaseSensitivity()) == 0 ? QString() : normalized;
    }

    QString BatchProcessSettings::defaultScriptDataDirectory() {
        auto documentsDirectory = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
        if (documentsDirectory.isEmpty()) {
            documentsDirectory = QDir::homePath();
        }
        return QDir::toNativeSeparators(QDir(documentsDirectory).filePath(QApplication::applicationName() + QStringLiteral("/Script Data")));
    }

    int BatchProcessSettings::maximumConsoleMessageCount() {
        Q_ASSERT(m_instance);
        return m_instance->m_maximumConsoleMessageCount;
    }

    void BatchProcessSettings::setMaximumConsoleMessageCount(int maximumMessageCount) {
        Q_ASSERT(m_instance);
        const auto boundedValue = boundedMaximumConsoleMessageCount(maximumMessageCount);
        if (m_instance->m_maximumConsoleMessageCount == boundedValue) {
            return;
        }
        m_instance->m_maximumConsoleMessageCount = boundedValue;
        qCInfo(lcBatchProcessSettings) << "Changed maximum retained JavaScript console messages to" << boundedValue;
        if (JavaScriptConsoleInterface::instance()) {
            JavaScriptConsoleInterface::instance()->setMaximumMessageCount(boundedValue);
        }
    }

    void BatchProcessSettings::load() {
        auto settings = Core::RuntimeInterface::settings();
        settings->beginGroup(staticMetaObject.className());
        m_customScriptDirectory = normalizedPath(settings->value(QStringLiteral("scriptDirectory")).toString());
        m_customScriptDataDirectory = normalizedPath(settings->value(QStringLiteral("scriptDataDirectory")).toString());
        bool ok = false;
        const auto storedMaximumMessageCount = settings->value(QStringLiteral("maximumConsoleMessageCount"), DefaultMaximumConsoleMessageCount).toInt(&ok);
        m_maximumConsoleMessageCount = boundedMaximumConsoleMessageCount(ok ? storedMaximumMessageCount : DefaultMaximumConsoleMessageCount);
        settings->endGroup();
        qCDebug(lcBatchProcessSettings) << "Loaded Batch Process settings" << m_customScriptDirectory << m_customScriptDataDirectory << m_maximumConsoleMessageCount;
    }

    void BatchProcessSettings::save() const {
        auto settings = Core::RuntimeInterface::settings();
        settings->beginGroup(staticMetaObject.className());
        settings->setValue(QStringLiteral("scriptDirectory"), m_customScriptDirectory);
        settings->setValue(QStringLiteral("scriptDataDirectory"), m_customScriptDataDirectory);
        settings->setValue(QStringLiteral("maximumConsoleMessageCount"), m_maximumConsoleMessageCount);
        settings->endGroup();
    }

}

#include "moc_BatchProcessSettings.cpp"
