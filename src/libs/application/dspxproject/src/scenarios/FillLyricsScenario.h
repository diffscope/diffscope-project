// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_DSPXPROJECT_FILLLYRICSSCENARIO_H
#define DIFFSCOPE_DSPXPROJECT_FILLLYRICSSCENARIO_H

#include <QObject>
#include <QScopedPointer>
#include <qqmlintegration.h>

#include <dspxproject/DocumentEditHints.h>
#include <dspxproject/dspxprojectglobal.h>

class QQuickWindow;

namespace DspxProject {

    class Document;
    class FillLyricsScenarioPrivate;
    class FillLyricsStrategy;

    /** Coordinates lyric replacement over selected notes in a transaction. */
    class DSPXPROJECT_EXPORT FillLyricsScenario : public QObject {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_DECLARE_PRIVATE(FillLyricsScenario)
    public:
        /** The available lyric tokenization modes for interaction implementations. */
        enum SplitMode {
            SplitMode_Auto, /**< Automatic language-dependent tokenization. */
            SplitMode_Character, /**< Individual characters. */
            SplitMode_Word, /**< Individual words. */
            SplitMode_Regex, /**< Regular expression tokenization. */
        };
        Q_ENUM(SplitMode)

        /**
         * Creates a scenario with borrowed dependencies.
         * @pre document and strategy are non-null and valid during use.
         */
        FillLyricsScenario(Document *document, FillLyricsStrategy *strategy, QObject *parent = nullptr);
        /** Destroys the scenario. */
        ~FillLyricsScenario() override;

        /**
         * Requests lyrics with an optional dialog window and applies accepted tokens in position order.
         * Performs no operation without selected notes. May extend to following notes
         * according to the strategy result, selecting those additional notes.
         */
        Q_INVOKABLE void fillLyrics(QQuickWindow *window, DocumentEdit::Hints hints = {}) const;

    private:
        QScopedPointer<FillLyricsScenarioPrivate> d_ptr;
    };

}

#endif // DIFFSCOPE_DSPXPROJECT_FILLLYRICSSCENARIO_H
