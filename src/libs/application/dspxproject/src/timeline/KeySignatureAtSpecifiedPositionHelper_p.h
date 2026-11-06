// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_DSPXPROJECT_KEYSIGNATUREATSPECIFIEDPOSITIONHELPER_P_H
#define DIFFSCOPE_DSPXPROJECT_KEYSIGNATUREATSPECIFIEDPOSITIONHELPER_P_H

#include <dspxproject/KeySignatureAtSpecifiedPositionHelper.h>

#include <QHash>
#include <QMap>

namespace DspxProject {

    class KeySignatureAtSpecifiedPositionHelperPrivate {
        Q_DECLARE_PUBLIC(KeySignatureAtSpecifiedPositionHelper)
    public:
        KeySignatureAtSpecifiedPositionHelper *q_ptr;

        void updateKeySignature();
        void rebuildKeySignatureLookup();
        void insertKeySignatureIntoLookup(dspx::KeySignature *item);
        void removeKeySignatureFromLookup(dspx::KeySignature *item);
        void updateKeySignaturePositionInLookup(dspx::KeySignature *item);
        void disconnectSequence();
        void connectSequence();
        void connectSequenceItem(dspx::KeySignature *item);
        void disconnectKeySignature();
        void connectKeySignature();

        int position = 0;
        dspx::KeySignatureSequence *keySignatureSequence = nullptr;
        dspx::KeySignature *keySignature = nullptr;
        QMap<int, dspx::KeySignature *> keySignatureLookup;
        QHash<dspx::KeySignature *, int> keySignaturePositions;
    };

}

#endif // DIFFSCOPE_DSPXPROJECT_KEYSIGNATUREATSPECIFIEDPOSITIONHELPER_P_H
