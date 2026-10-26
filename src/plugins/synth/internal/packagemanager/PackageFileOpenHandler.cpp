// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "PackageFileOpenHandler.h"

#include <QList>
#include <QUrl>

#include <synth/internal/PackageManagerController.h>

namespace Synth::Internal {

    PackageFileOpenHandler::PackageFileOpenHandler(PackageManagerController *packageManager, QObject *parent)
        : Core::IFileOpenHandler(parent), m_packageManager(packageManager) {
    }

    PackageFileOpenHandler::~PackageFileOpenHandler() = default;

    QSet<qsizetype> PackageFileOpenHandler::preflight(const QStringList &filePaths, QWindow *) const {
        QSet<qsizetype> indexes;
        for (qsizetype index = 0; index < filePaths.size(); ++index) {
            if (filePaths.at(index).endsWith(QStringLiteral(".dspk"), Qt::CaseInsensitive)) {
                indexes.insert(index);
            }
        }
        return indexes;
    }

    QSet<qsizetype> PackageFileOpenHandler::openFiles(const QStringList &filePaths, QWindow *parentWindow) {
        QSet<qsizetype> indexes;
        QList<QUrl> files;
        for (qsizetype index = 0; index < filePaths.size(); ++index) {
            if (filePaths.at(index).endsWith(QStringLiteral(".dspk"), Qt::CaseInsensitive)) {
                indexes.insert(index);
                files.append(QUrl::fromLocalFile(filePaths.at(index)));
            }
        }
        if (!files.isEmpty()) {
            m_packageManager->installPackages(files, parentWindow);
        }
        return indexes;
    }

}

#include "moc_PackageFileOpenHandler.cpp"
