// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#ifndef DIFFSCOPE_COREPLUGIN_PROJECTFILEOPENHANDLER_H
#define DIFFSCOPE_COREPLUGIN_PROJECTFILEOPENHANDLER_H

#include <coreplugin/IFileOpenHandler.h>

namespace Core::Internal {

    class ProjectFileOpenHandler : public IFileOpenHandler {
        Q_OBJECT
    public:
        explicit ProjectFileOpenHandler(QObject *parent = nullptr);
        ~ProjectFileOpenHandler() override;

        QSet<qsizetype> preflight(const QStringList &filePaths, QWindow *parentWindow) const override;
        QSet<qsizetype> openFiles(const QStringList &filePaths, QWindow *parentWindow) override;
    };

}

#endif // DIFFSCOPE_COREPLUGIN_PROJECTFILEOPENHANDLER_H
