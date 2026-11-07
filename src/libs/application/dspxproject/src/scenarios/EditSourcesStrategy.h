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

    /** Supplies source selection and diagnostics. Window arguments are optional dialog parents. */
    class DSPXPROJECT_EXPORT EditSourcesStrategy : public QObject {
        Q_OBJECT
    public:
        /** Creates a strategy with an optional QObject parent. */
        explicit EditSourcesStrategy(QObject *parent = nullptr);
        /** Destroys the strategy. */
        ~EditSourcesStrategy() override;

        /** Returns the accepted source selection, or nullopt on cancellation. */
        virtual std::optional<SourcesSelection> requestSources(const SourcesSelection &initial, QQuickWindow *window, DocumentEdit::Hints hints) = 0;
        /** Reports note languages unsupported by the applied singer. */
        virtual void reportUnsupportedNoteLanguages(QQuickWindow *window) = 0;
        /** Reports a conflicting operation that prevents a transaction. */
        virtual void reportOperationConflict() = 0;
    };

}

#endif // DIFFSCOPE_DSPXPROJECT_EDITSOURCESSTRATEGY_H
