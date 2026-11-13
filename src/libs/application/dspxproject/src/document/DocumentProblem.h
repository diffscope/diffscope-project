// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_DSPXPROJECT_DOCUMENTPROBLEM_H
#define DIFFSCOPE_DSPXPROJECT_DOCUMENTPROBLEM_H

#include <QFlags>
#include <QPointer>
#include <QString>
#include <QStringList>
#include <QVariant>

#include <dspxmodelORM/EntityObject.h>

namespace DspxProject {

    enum class DocumentProblemId : quint64 {
        Invalid = 0,
    };

    enum class DocumentProblemOperationId : quint64 {
        Invalid = 0,
    };

    enum class DocumentProblemSeverity {
        Error = 0x01,
        Warning = 0x02,
        Notice = 0x04,
    };
    Q_DECLARE_FLAGS(DocumentProblemSeverities, DocumentProblemSeverity)
    Q_DECLARE_OPERATORS_FOR_FLAGS(DocumentProblemSeverities)

    enum class DocumentProblemScope {
        Direct,
        IncludeDescendants,
    };

    struct DocumentProblemInfo {
        QString type;
        DocumentProblemSeverity severity = DocumentProblemSeverity::Error;
        QString message;
        QPointer<dspx::EntityObject> entity;
        QVariant data;
    };

    struct DocumentProblemReport {
        QString key;
        DocumentProblemInfo info;
    };

    struct DocumentProblem {
        DocumentProblemId id = DocumentProblemId::Invalid;
        DocumentProblemInfo info;
    };

    struct DocumentProblemFilter {
        DocumentProblemScope scope = DocumentProblemScope::IncludeDescendants;
        DocumentProblemSeverities severities{
            DocumentProblemSeverity::Error,
            DocumentProblemSeverity::Warning,
            DocumentProblemSeverity::Notice,
        };
        QStringList types;
    };

    struct DocumentProblemAction {
        DocumentProblemOperationId id = DocumentProblemOperationId::Invalid;
        QString text;
    };

}

#endif // DIFFSCOPE_DSPXPROJECT_DOCUMENTPROBLEM_H
