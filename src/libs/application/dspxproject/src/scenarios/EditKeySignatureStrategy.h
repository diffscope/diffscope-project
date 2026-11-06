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

    class DSPXPROJECT_EXPORT EditKeySignatureStrategy : public QObject {
        Q_OBJECT
    public:
        explicit EditKeySignatureStrategy(QObject *parent = nullptr);
        ~EditKeySignatureStrategy() override;

        struct Input {
            int position;
            bool insert;
            int tonality;
            int mode;
            int accidentalType;
        };

        virtual std::optional<Input> requestKeySignature(const Input &initial, SVS::MusicTimeline *timeline, QQuickWindow *window, DocumentEdit::Hints hints) = 0;
        virtual void reportOperationConflict() = 0;
    };

}

#endif // DIFFSCOPE_DSPXPROJECT_EDITKEYSIGNATURESTRATEGY_H
