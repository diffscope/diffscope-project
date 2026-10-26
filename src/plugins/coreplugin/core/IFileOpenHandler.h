// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#ifndef DIFFSCOPE_COREPLUGIN_IFILEOPENHANDLER_H
#define DIFFSCOPE_COREPLUGIN_IFILEOPENHANDLER_H

#include <QObject>
#include <QScopedPointer>
#include <QSet>
#include <QStringList>

#include <coreplugin/coreglobal.h>

class QWindow;

namespace Core {

    class IFileOpenHandlerPrivate;

    class CORE_EXPORT IFileOpenHandler : public QObject {
        Q_OBJECT
        Q_DECLARE_PRIVATE(IFileOpenHandler)
    public:
        explicit IFileOpenHandler(QObject *parent = nullptr);
        ~IFileOpenHandler() override;

        // Returns accepted indexes without side effects. The default implementation accepts no files.
        virtual QSet<qsizetype> preflight(const QStringList &filePaths, QWindow *parentWindow) const;

        // Returns claimed indexes in this batch, including failed or canceled operations.
        // Handles every file assigned by preflight. Unassigned files may also be claimed.
        virtual QSet<qsizetype> openFiles(const QStringList &filePaths, QWindow *parentWindow) = 0;

    private:
        QScopedPointer<IFileOpenHandlerPrivate> d_ptr;
    };

}

#endif // DIFFSCOPE_COREPLUGIN_IFILEOPENHANDLER_H
