// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_BATCHPROCESS_SCRIPTMETADATA_H
#define DIFFSCOPE_BATCHPROCESS_SCRIPTMETADATA_H

#include <QString>

#include <batchprocess/batchprocessglobal.h>

namespace BatchProcess {

    namespace Internal {
        class BatchProcessRuntime;
    }

    class BATCH_PROCESS_EXPORT ScriptMetadata {
    public:
        ScriptMetadata();

        QString id() const;
        QString name() const;
        QString version() const;
        QString description() const;
        QString author() const;
        bool isValid() const;

    private:
        friend class Internal::BatchProcessRuntime;

        QString m_id;
        QString m_name;
        QString m_version;
        QString m_description;
        QString m_author;
    };

}

#endif // DIFFSCOPE_BATCHPROCESS_SCRIPTMETADATA_H
