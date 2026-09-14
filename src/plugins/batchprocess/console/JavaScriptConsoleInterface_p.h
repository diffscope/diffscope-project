// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_BATCHPROCESS_JAVASCRIPTCONSOLEINTERFACE_P_H
#define DIFFSCOPE_BATCHPROCESS_JAVASCRIPTCONSOLEINTERFACE_P_H

#include <batchprocess/JavaScriptConsoleInterface.h>

#include <QAbstractListModel>
#include <QByteArray>
#include <QHash>
#include <QList>
#include <QVariant>

namespace BatchProcess {

    struct JavaScriptConsoleMessage {
        JavaScriptConsoleInterface::Level level{};
        QString text;
        QUrl fileUrl;
        int line{-1};
        int column{-1};
        QList<JavaScriptConsoleStackFrame> stackTrace;
    };

    class JavaScriptConsoleModel : public QAbstractListModel {
    public:
        explicit JavaScriptConsoleModel(QObject *parent = nullptr);

        int rowCount(const QModelIndex &parent = {}) const override;
        QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
        QHash<int, QByteArray> roleNames() const override;

        void appendMessage(JavaScriptConsoleMessage message);
        void clear();
        void setMaximumMessageCount(int maximumMessageCount);

    private:
        void trimToMaximum();

        QList<JavaScriptConsoleMessage> m_messages;
        int m_maximumMessageCount{4096};
    };

    class JavaScriptConsoleInterfacePrivate {
        Q_DECLARE_PUBLIC(JavaScriptConsoleInterface)
    public:
        explicit JavaScriptConsoleInterfacePrivate(JavaScriptConsoleInterface *q);

        JavaScriptConsoleInterface *q_ptr;
        JavaScriptConsoleModel *model{};
    };

}

#endif // DIFFSCOPE_BATCHPROCESS_JAVASCRIPTCONSOLEINTERFACE_P_H
