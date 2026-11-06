// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_DSPXPROJECT_EDITSOURCESSTRATEGY_H
#define DIFFSCOPE_DSPXPROJECT_EDITSOURCESSTRATEGY_H

#include <optional>

#include <QObject>

#include <dspxproject/DocumentEditHints.h>
#include <dspxproject/SourcesSelection.h>
#include <dspxproject/dspxprojectglobal.h>

class QQuickWindow;

namespace DspxProject {

    class DSPXPROJECT_EXPORT EditSourcesStrategy : public QObject {
        Q_OBJECT
    public:
        explicit EditSourcesStrategy(QObject *parent = nullptr);
        ~EditSourcesStrategy() override;

        virtual std::optional<SourcesSelection> requestSources(const SourcesSelection &initial, QQuickWindow *window, DocumentEdit::Hints hints) = 0;
        virtual void reportUnsupportedNoteLanguages(QQuickWindow *window) = 0;
        virtual void reportOperationConflict() = 0;
    };

}

#endif // DIFFSCOPE_DSPXPROJECT_EDITSOURCESSTRATEGY_H
