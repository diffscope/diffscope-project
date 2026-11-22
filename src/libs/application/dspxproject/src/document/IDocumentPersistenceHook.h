// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_DSPXPROJECT_IDOCUMENTPERSISTENCEHOOK_H
#define DIFFSCOPE_DSPXPROJECT_IDOCUMENTPERSISTENCEHOOK_H

#include <QObject>
#include <QString>

#include <dspxproject/dspxprojectglobal.h>

namespace opendspx {
    struct Model;
}

namespace DspxProject {

    class ProjectDocumentContext;

    /**
     * Transforms the OpenDSPX model synchronously during project persistence.
     * The model reference is valid only during the callback.
     */
    class DSPXPROJECT_EXPORT IDocumentPersistenceHook : public QObject {
        Q_OBJECT
    public:
        explicit IDocumentPersistenceHook(QObject *parent = nullptr);
        ~IDocumentPersistenceHook() override;

        /**
         * Updates the input model before the document is initialized.
         * recoverySourceDirectory is the source log directory during document log recovery and is empty for other loads.
         */
        virtual void beforeLoadModel(ProjectDocumentContext &context, opendspx::Model &model, const QString &recoverySourceDirectory) = 0;

        /** Updates the extracted model before serialization. */
        virtual void afterSaveModel(ProjectDocumentContext &context, opendspx::Model &model) = 0;
    };

}

#endif // DIFFSCOPE_DSPXPROJECT_IDOCUMENTPERSISTENCEHOOK_H
