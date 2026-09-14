// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_BATCHPROCESS_JAVASCRIPTCONSOLEINTERFACE_H
#define DIFFSCOPE_BATCHPROCESS_JAVASCRIPTCONSOLEINTERFACE_H

#include <QAbstractItemModel>
#include <QList>
#include <QMetaType>
#include <QObject>
#include <QScopedPointer>
#include <QString>
#include <QUrl>

#include <batchprocess/batchprocessglobal.h>

namespace BatchProcess {

    struct BATCH_PROCESS_EXPORT JavaScriptConsoleStackFrame {
        QString functionName;
        QUrl fileUrl;
        int line{-1};
        int column{-1};
    };

    namespace Internal {
        class BatchProcessSettings;
        class BatchProcessPlugin;
    }

    class JavaScriptConsoleInterfacePrivate;

    class BATCH_PROCESS_EXPORT JavaScriptConsoleInterface : public QObject {
        Q_OBJECT
        Q_DECLARE_PRIVATE(JavaScriptConsoleInterface)
    public:
        enum Level {
            Debug,
            Log,
            Info,
            Warning,
            Error,
        };
        Q_ENUM(Level)

        enum Role {
            LevelRole = Qt::UserRole + 1,
            TextRole,
            FileUrlRole,
            LineRole,
            ColumnRole,
            StackTraceRole,
        };
        Q_ENUM(Role)

        ~JavaScriptConsoleInterface() override;

        static JavaScriptConsoleInterface *instance();

        /** The optional stack trace is rendered as linked source frames after the message text. */
        void appendMessage(
            Level level,
            const QString &text,
            const QUrl &fileUrl = {},
            int line = -1,
            int column = -1,
            const QList<JavaScriptConsoleStackFrame> &stackTrace = {}
        );
        void clear();
        QAbstractItemModel *model() const;

    private:
        friend class Internal::BatchProcessPlugin;
        friend class Internal::BatchProcessSettings;

        explicit JavaScriptConsoleInterface(QObject *parent = nullptr);
        void setMaximumMessageCount(int maximumMessageCount);

        QScopedPointer<JavaScriptConsoleInterfacePrivate> d_ptr;
    };

}

Q_DECLARE_METATYPE(BatchProcess::JavaScriptConsoleStackFrame)

#endif // DIFFSCOPE_BATCHPROCESS_JAVASCRIPTCONSOLEINTERFACE_H
