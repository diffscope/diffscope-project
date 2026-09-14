// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_BATCHPROCESS_FILESYSTEMPATHUTILS_H
#define DIFFSCOPE_BATCHPROCESS_FILESYSTEMPATHUTILS_H

#include <QString>

namespace BatchProcess::Internal {

    QString normalizedFileSystemPath(const QString &path);
    bool resolveFileSystemPath(const QString &path, const QString &basePath, QString *result, QString *errorMessage);
    bool normalizeFileSystemPattern(const QString &pattern, const QString &basePath, QString *result, QString *errorMessage);
    bool fileSystemPatternHasMagic(const QString &pattern);
    bool matchesFileSystemPattern(const QString &path, const QString &pattern, QString *errorMessage = nullptr);
    bool recursivePatternContains(const QString &existingPattern, const QString &requestedPattern);
    bool isPathWithin(const QString &rootPath, const QString &path);
    QString safeScriptDirectoryName(const QString &scriptId);

}

#endif // DIFFSCOPE_BATCHPROCESS_FILESYSTEMPATHUTILS_H
