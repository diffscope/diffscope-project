// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "JavaScriptConsoleInterface.h"
#include "JavaScriptConsoleInterface_p.h"

#include <utility>

#include <QLoggingCategory>

#include <batchprocess/internal/BatchProcessSettings.h>

namespace BatchProcess {

    Q_STATIC_LOGGING_CATEGORY(lcJavaScriptConsoleInterface, "diffscope.batchprocess.javascriptconsole.interface")

    namespace {

        JavaScriptConsoleInterface *s_instance{};

    }

    JavaScriptConsoleModel::JavaScriptConsoleModel(QObject *parent) : QAbstractListModel(parent) {
    }

    int JavaScriptConsoleModel::rowCount(const QModelIndex &parent) const {
        return parent.isValid() ? 0 : m_messages.size();
    }

    QVariant JavaScriptConsoleModel::data(const QModelIndex &index, int role) const {
        if (!index.isValid() || index.row() < 0 || index.row() >= m_messages.size()) {
            return {};
        }

        const auto &message = m_messages.at(index.row());
        switch (role) {
            case Qt::DisplayRole:
            case JavaScriptConsoleInterface::TextRole:
                return message.text;
            case JavaScriptConsoleInterface::LevelRole:
                return static_cast<int>(message.level);
            case JavaScriptConsoleInterface::FileUrlRole:
                return message.fileUrl;
            case JavaScriptConsoleInterface::LineRole:
                return message.line;
            case JavaScriptConsoleInterface::ColumnRole:
                return message.column;
            case JavaScriptConsoleInterface::StackTraceRole:
                return QVariant::fromValue(message.stackTrace);
            default:
                return {};
        }
    }

    QHash<int, QByteArray> JavaScriptConsoleModel::roleNames() const {
        return {
            {JavaScriptConsoleInterface::LevelRole, "level"},
            {JavaScriptConsoleInterface::TextRole, "text"},
            {JavaScriptConsoleInterface::FileUrlRole, "fileUrl"},
            {JavaScriptConsoleInterface::LineRole, "line"},
            {JavaScriptConsoleInterface::ColumnRole, "column"},
            {JavaScriptConsoleInterface::StackTraceRole, "stackTrace"},
        };
    }

    void JavaScriptConsoleModel::appendMessage(JavaScriptConsoleMessage message) {
        if (m_messages.size() >= m_maximumMessageCount) {
            const auto removeCount = m_messages.size() - m_maximumMessageCount + 1;
            beginRemoveRows({}, 0, removeCount - 1);
            m_messages.remove(0, removeCount);
            endRemoveRows();
        }

        const auto row = m_messages.size();
        beginInsertRows({}, row, row);
        m_messages.append(std::move(message));
        endInsertRows();
    }

    void JavaScriptConsoleModel::clear() {
        if (m_messages.isEmpty()) {
            return;
        }
        beginRemoveRows({}, 0, m_messages.size() - 1);
        m_messages.clear();
        endRemoveRows();
    }

    void JavaScriptConsoleModel::setMaximumMessageCount(int maximumMessageCount) {
        m_maximumMessageCount = maximumMessageCount;
        trimToMaximum();
    }

    void JavaScriptConsoleModel::trimToMaximum() {
        const auto removeCount = m_messages.size() - m_maximumMessageCount;
        if (removeCount <= 0) {
            return;
        }
        beginRemoveRows({}, 0, removeCount - 1);
        m_messages.remove(0, removeCount);
        endRemoveRows();
    }

    JavaScriptConsoleInterfacePrivate::JavaScriptConsoleInterfacePrivate(JavaScriptConsoleInterface *q) : q_ptr(q) {
        model = new JavaScriptConsoleModel(q);
    }

    JavaScriptConsoleInterface::JavaScriptConsoleInterface(QObject *parent)
        : QObject(parent), d_ptr(new JavaScriptConsoleInterfacePrivate(this)) {
        Q_ASSERT(!s_instance);
        s_instance = this;
        if (Internal::BatchProcessSettings::instance()) {
            setMaximumMessageCount(Internal::BatchProcessSettings::maximumConsoleMessageCount());
        }
    }

    JavaScriptConsoleInterface::~JavaScriptConsoleInterface() {
        if (s_instance == this) {
            s_instance = nullptr;
        }
    }

    JavaScriptConsoleInterface *JavaScriptConsoleInterface::instance() {
        return s_instance;
    }

    void JavaScriptConsoleInterface::appendMessage(Level level, const QString &text, const QUrl &fileUrl, int line, int column, const QList<JavaScriptConsoleStackFrame> &stackTrace) {
        Q_D(JavaScriptConsoleInterface);
        if (line < 1) {
            line = -1;
            column = -1;
        } else if (column < 1) {
            column = -1;
        }
        auto normalizedStackTrace = stackTrace;
        for (auto &frame : normalizedStackTrace) {
            if (frame.line < 1) {
                frame.line = -1;
                frame.column = -1;
            } else if (frame.column < 1) {
                frame.column = -1;
            }
        }
        d->model->appendMessage({level, text, fileUrl, line, column, std::move(normalizedStackTrace)});
        qCDebug(lcJavaScriptConsoleInterface) << "Appended JavaScript console message" << level << fileUrl << line << column << text;
    }

    void JavaScriptConsoleInterface::clear() {
        Q_D(JavaScriptConsoleInterface);
        const auto removedCount = d->model->rowCount();
        d->model->clear();
        qCInfo(lcJavaScriptConsoleInterface) << "Cleared JavaScript console messages" << removedCount;
    }

    QAbstractItemModel *JavaScriptConsoleInterface::model() const {
        Q_D(const JavaScriptConsoleInterface);
        return d->model;
    }

    void JavaScriptConsoleInterface::setMaximumMessageCount(int maximumMessageCount) {
        Q_D(JavaScriptConsoleInterface);
        d->model->setMaximumMessageCount(maximumMessageCount);
        qCDebug(lcJavaScriptConsoleInterface) << "Applied JavaScript console message limit" << maximumMessageCount;
    }

}

#include "moc_JavaScriptConsoleInterface.cpp"
