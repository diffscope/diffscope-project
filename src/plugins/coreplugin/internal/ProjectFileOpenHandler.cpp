// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#include "ProjectFileOpenHandler.h"

#include <QPointer>
#include <QWindow>

#include <coreplugin/CoreInterface.h>
#include <coreplugin/internal/ProjectWindowNavigatorAddOn.h>

namespace Core::Internal {

    ProjectFileOpenHandler::ProjectFileOpenHandler(QObject *parent) : IFileOpenHandler(parent) {
    }

    ProjectFileOpenHandler::~ProjectFileOpenHandler() = default;

    QSet<qsizetype> ProjectFileOpenHandler::preflight(const QStringList &filePaths, QWindow *) const {
        QSet<qsizetype> indexes;
        for (qsizetype index = 0; index < filePaths.size(); ++index) {
            if (filePaths.at(index).endsWith(QStringLiteral(".dspx"), Qt::CaseInsensitive)) {
                indexes.insert(index);
            }
        }
        return indexes;
    }

    QSet<qsizetype> ProjectFileOpenHandler::openFiles(const QStringList &filePaths, QWindow *parentWindow) {
        const QPointer<QWindow> parent(parentWindow);
        QSet<qsizetype> indexes;
        for (qsizetype index = 0; index < filePaths.size(); ++index) {
            if (auto windowInterface = CoreInterface::openProjectFile(filePaths.at(index), parent)) {
                ProjectWindowNavigatorAddOn::raiseWindow(windowInterface);
            }
            indexes.insert(index);
        }
        return indexes;
    }

}

#include "moc_ProjectFileOpenHandler.cpp"
