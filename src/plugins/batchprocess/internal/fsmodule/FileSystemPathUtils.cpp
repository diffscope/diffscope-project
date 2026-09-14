// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "FileSystemPathUtils.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>
#include <QUrl>

#include <batchprocess/FileSystemAccessInterface.h>

namespace BatchProcess::Internal {

    namespace {

        constexpr Qt::CaseSensitivity pathCaseSensitivity() {
#if defined(Q_OS_WIN)
            return Qt::CaseInsensitive;
#else
            return Qt::CaseSensitive;
#endif
        }

        QString forwardSlashes(QString path) {
            return path.replace(QLatin1Char('\\'), QLatin1Char('/'));
        }

        bool appendSegmentExpression(const QString &segment, QString *expression, QString *errorMessage) {
            for (qsizetype index = 0; index < segment.size(); ++index) {
                const auto character = segment.at(index);
                if (character == QLatin1Char('*')) {
                    expression->append(QStringLiteral("[^/]*"));
                    while (index + 1 < segment.size() && segment.at(index + 1) == QLatin1Char('*')) {
                        ++index;
                    }
                    continue;
                }
                if (character == QLatin1Char('?')) {
                    expression->append(QStringLiteral("[^/]"));
                    continue;
                }
                if (character == QLatin1Char('[')) {
                    const auto closingIndex = segment.indexOf(QLatin1Char(']'), index + 1);
                    if (closingIndex < 0 || closingIndex == index + 1) {
                        if (errorMessage) {
                            *errorMessage = FileSystemAccessInterface::tr("Invalid unterminated character class in file-system pattern.");
                        }
                        return false;
                    }
                    auto contents = segment.mid(index + 1, closingIndex - index - 1);
                    QString characterClass = QStringLiteral("[");
                    if (contents.startsWith(QLatin1Char('!'))) {
                        characterClass.append(QLatin1Char('^'));
                        contents.removeFirst();
                    }
                    if (contents.isEmpty() || contents.contains(QLatin1Char('/')) || contents.contains(QLatin1Char('\\'))) {
                        if (errorMessage) {
                            *errorMessage = FileSystemAccessInterface::tr("Invalid character class in file-system pattern.");
                        }
                        return false;
                    }
                    if (contents.startsWith(QLatin1Char('^'))) {
                        characterClass.append(QStringLiteral("\\^"));
                        contents.removeFirst();
                    }
                    characterClass.append(contents);
                    characterClass.append(QLatin1Char(']'));
                    expression->append(characterClass);
                    index = closingIndex;
                    continue;
                }
                expression->append(QRegularExpression::escape(QString(character)));
            }
            return true;
        }

        bool globExpression(const QString &pattern, QString *expression, QString *errorMessage) {
            const auto normalizedPattern = forwardSlashes(pattern);
            QString root;
            QString remainder = normalizedPattern;
#if defined(Q_OS_WIN)
            static const QRegularExpression driveExpression(QStringLiteral("^[A-Za-z]:/"));
            const auto driveMatch = driveExpression.match(remainder);
            if (driveMatch.hasMatch()) {
                root = remainder.left(3);
                remainder.remove(0, 3);
            } else if (remainder.startsWith(QStringLiteral("//"))) {
                root = QStringLiteral("//");
                remainder.remove(0, 2);
            }
#else
            if (remainder.startsWith(QLatin1Char('/'))) {
                root = QStringLiteral("/");
                remainder.removeFirst();
            }
#endif

            const auto segments = remainder.split(QLatin1Char('/'), Qt::SkipEmptyParts);
            QString result = QStringLiteral("\\A") + QRegularExpression::escape(root);
            bool previousGlobStarConsumesSeparator = false;
            for (qsizetype index = 0; index < segments.size(); ++index) {
                const auto &segment = segments.at(index);
                const auto hasFollowing = index + 1 < segments.size();
                if (segment == QStringLiteral("**")) {
                    if (hasFollowing) {
                        if (index > 0 && !previousGlobStarConsumesSeparator) {
                            result.append(QLatin1Char('/'));
                        }
                        result.append(QStringLiteral("(?:[^/]+/)*"));
                        previousGlobStarConsumesSeparator = true;
                    } else if (index > 0 && !previousGlobStarConsumesSeparator) {
                        result.append(QStringLiteral("(?:/.*)?"));
                    } else {
                        result.append(QStringLiteral(".*"));
                    }
                    continue;
                }

                if (index > 0 && !previousGlobStarConsumesSeparator) {
                    result.append(QLatin1Char('/'));
                }
                if (!appendSegmentExpression(segment, &result, errorMessage)) {
                    return false;
                }
                previousGlobStarConsumesSeparator = false;
            }
            result.append(QStringLiteral("\\z"));
            *expression = result;
            return true;
        }

        QString fixedPatternPrefix(const QString &pattern) {
            qsizetype magicIndex = -1;
            for (const auto character : {QLatin1Char('?'), QLatin1Char('*'), QLatin1Char('[')}) {
                const auto index = pattern.indexOf(character);
                if (index >= 0 && (magicIndex < 0 || index < magicIndex)) {
                    magicIndex = index;
                }
            }
            if (magicIndex < 0) {
                return pattern;
            }
            auto prefix = pattern.left(magicIndex);
            const auto separatorIndex = prefix.lastIndexOf(QLatin1Char('/'));
            if (separatorIndex >= 0) {
                prefix.truncate(separatorIndex);
            }
            return prefix;
        }

    }

    QString normalizedFileSystemPath(const QString &path) {
        if (path.isEmpty()) {
            return {};
        }
        return forwardSlashes(QDir::cleanPath(path));
    }

    bool resolveFileSystemPath(const QString &path, const QString &basePath, QString *result, QString *errorMessage) {
        if (!result || path.trimmed().isEmpty() || path.contains(QChar::Null)) {
            if (errorMessage) {
                *errorMessage = FileSystemAccessInterface::tr("The file-system path is empty or invalid.");
            }
            return false;
        }
        QString absolutePath;
        if (QDir::isAbsolutePath(path)) {
            absolutePath = path;
        } else {
            if (basePath.isEmpty()) {
                if (errorMessage) {
                    *errorMessage = FileSystemAccessInterface::tr("A relative file-system path cannot be used because the current script has no root directory.");
                }
                return false;
            }
            absolutePath = QDir(basePath).filePath(path);
        }
        absolutePath = QFileInfo(QDir::cleanPath(absolutePath)).absoluteFilePath();
        if (!QDir::isAbsolutePath(absolutePath)) {
            if (errorMessage) {
                *errorMessage = FileSystemAccessInterface::tr("The file-system path could not be resolved to an absolute path.");
            }
            return false;
        }
        *result = normalizedFileSystemPath(absolutePath);
        return true;
    }

    bool normalizeFileSystemPattern(const QString &pattern, const QString &basePath, QString *result, QString *errorMessage) {
        if (!resolveFileSystemPath(pattern, basePath, result, errorMessage)) {
            return false;
        }
        QString expression;
        if (!globExpression(*result, &expression, errorMessage)) {
            return false;
        }
        QRegularExpression::PatternOptions options = QRegularExpression::NoPatternOption;
#if defined(Q_OS_WIN)
        options |= QRegularExpression::CaseInsensitiveOption;
#endif
        const QRegularExpression regularExpression(expression, options);
        if (!regularExpression.isValid()) {
            if (errorMessage) {
                *errorMessage = regularExpression.errorString();
            }
            return false;
        }
        return true;
    }

    bool fileSystemPatternHasMagic(const QString &pattern) {
        return pattern.contains(QLatin1Char('*')) || pattern.contains(QLatin1Char('?')) || pattern.contains(QLatin1Char('['));
    }

    bool matchesFileSystemPattern(const QString &path, const QString &pattern, QString *errorMessage) {
        QString expression;
        if (!globExpression(pattern, &expression, errorMessage)) {
            return false;
        }
        QRegularExpression::PatternOptions options = QRegularExpression::NoPatternOption;
#if defined(Q_OS_WIN)
        options |= QRegularExpression::CaseInsensitiveOption;
#endif
        const QRegularExpression regularExpression(expression, options);
        if (!regularExpression.isValid()) {
            if (errorMessage) {
                *errorMessage = regularExpression.errorString();
            }
            return false;
        }
        return regularExpression.match(normalizedFileSystemPath(path)).hasMatch();
    }

    bool recursivePatternContains(const QString &existingPattern, const QString &requestedPattern) {
        if (!existingPattern.endsWith(QStringLiteral("/**"))) {
            return false;
        }
        auto root = existingPattern;
        root.chop(3);
        const auto requestedPrefix = fixedPatternPrefix(requestedPattern);
        return isPathWithin(root, requestedPrefix);
    }

    bool isPathWithin(const QString &rootPath, const QString &path) {
        const auto root = normalizedFileSystemPath(rootPath);
        const auto candidate = normalizedFileSystemPath(path);
        if (root.compare(candidate, pathCaseSensitivity()) == 0) {
            return true;
        }
        auto prefix = root;
        if (!prefix.endsWith(QLatin1Char('/'))) {
            prefix.append(QLatin1Char('/'));
        }
        return candidate.startsWith(prefix, pathCaseSensitivity());
    }

    QString safeScriptDirectoryName(const QString &scriptId) {
        auto encoded = QString::fromLatin1(QUrl::toPercentEncoding(scriptId, QByteArrayLiteral("-_.")));
        static const QRegularExpression reservedWindowsName(QStringLiteral("^(?:CON|PRN|AUX|NUL|COM[1-9]|LPT[1-9])(?:\\..*)?$"), QRegularExpression::CaseInsensitiveOption);
        if (encoded.isEmpty() || encoded == QStringLiteral(".") || encoded == QStringLiteral("..") || reservedWindowsName.match(encoded).hasMatch()) {
            encoded.prepend(QLatin1Char('_'));
        }
        if (encoded.size() <= 120) {
            return encoded;
        }
        const auto hash = QString::fromLatin1(QCryptographicHash::hash(scriptId.toUtf8(), QCryptographicHash::Sha256).toHex().left(24));
        return encoded.left(88) + QLatin1Char('-') + hash;
    }

}
