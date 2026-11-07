// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_DSPXPROJECT_EDITKEYSIGNATURESTRATEGY_H
#define DIFFSCOPE_DSPXPROJECT_EDITKEYSIGNATURESTRATEGY_H

#include <optional>

#include <QObject>

#include <dspxproject/DocumentEditHints.h>
#include <dspxproject/dspxprojectglobal.h>

class QQuickWindow;

namespace SVS {
    class MusicTimeline;
}

namespace DspxProject {

    /** Supplies key signature interaction with optional window parents and required timelines. */
    class DSPXPROJECT_EXPORT EditKeySignatureStrategy : public QObject {
        Q_OBJECT
    public:
        /** Creates a strategy with an optional QObject parent. */
        explicit EditKeySignatureStrategy(QObject *parent = nullptr);
        /** Destroys the strategy. */
        ~EditKeySignatureStrategy() override;

        /** The initial or accepted key signature values. */
        struct Input {
            /** The project tick position. */
            int position;
            /** Whether to insert at position instead of modifying the effective item. */
            bool insert;
            /** The pitch class from 0 through 11. */
            int tonality;
            /** The 12-bit scale mask. */
            int mode;
            /** The accidental type, 0 for flats or 1 for sharps. */
            int accidentalType;
        };

        /**
         * Returns accepted values, or nullopt on cancellation.
         * @pre timeline is non-null and valid during the interaction.
         */
        virtual std::optional<Input> requestKeySignature(const Input &initial, SVS::MusicTimeline *timeline, QQuickWindow *window, DocumentEdit::Hints hints) = 0;
        /** Reports a conflicting operation that prevents a transaction. */
        virtual void reportOperationConflict() = 0;
    };

}

#endif // DIFFSCOPE_DSPXPROJECT_EDITKEYSIGNATURESTRATEGY_H
