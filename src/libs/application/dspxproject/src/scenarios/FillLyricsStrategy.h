// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_DSPXPROJECT_FILLLYRICSSTRATEGY_H
#define DIFFSCOPE_DSPXPROJECT_FILLLYRICSSTRATEGY_H

#include <optional>

#include <QObject>
#include <QStringList>

#include <dspxproject/DocumentEditHints.h>
#include <dspxproject/dspxprojectglobal.h>

class QQuickWindow;

namespace DspxProject {

    /** Supplies lyric interaction. Window arguments are optional dialog parents. */
    class DSPXPROJECT_EXPORT FillLyricsStrategy : public QObject {
        Q_OBJECT
    public:
        /** Creates a strategy with an optional QObject parent. */
        explicit FillLyricsStrategy(QObject *parent = nullptr);
        /** Destroys the strategy. */
        ~FillLyricsStrategy() override;

        /** The accepted lyric tokens and application extent. */
        struct Result {
            /** The lyric tokens in note order. An empty list performs no edit. */
            QStringList lyrics;
            /** Whether application is restricted to the original selected notes. */
            bool truncateToSelection;
        };

        /** Returns accepted lyrics and extent, or nullopt on cancellation. */
        virtual std::optional<Result> requestLyrics(const QStringList &initialLyrics, bool truncateToSelection,
                                                    QQuickWindow *window, DocumentEdit::Hints hints) = 0;
        /** Reports a conflicting operation that prevents a transaction. */
        virtual void reportOperationConflict() = 0;
    };

}

#endif // DIFFSCOPE_DSPXPROJECT_FILLLYRICSSTRATEGY_H
