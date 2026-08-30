// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#ifndef DIFFSCOPE_COREPLUGIN_KEYSIGNATUREATSPECIFIEDPOSITIONHELPER_H
#define DIFFSCOPE_COREPLUGIN_KEYSIGNATUREATSPECIFIEDPOSITIONHELPER_H

#include <QHash>
#include <QMap>
#include <QObject>
#include <qqmlintegration.h>

#include <coreplugin/coreglobal.h>

namespace dspx {
    class KeySignature;
    class KeySignatureSequence;
}

namespace Core::Internal {

    class CORE_EXPORT KeySignatureAtSpecifiedPositionHelper : public QObject {
        Q_OBJECT
        QML_ELEMENT
        Q_PROPERTY(int position READ position WRITE setPosition NOTIFY positionChanged)
        Q_PROPERTY(dspx::KeySignatureSequence *keySignatureSequence READ keySignatureSequence WRITE setKeySignatureSequence NOTIFY keySignatureSequenceChanged)
        Q_PROPERTY(dspx::KeySignature *keySignature READ keySignature NOTIFY keySignatureChanged)
        Q_PROPERTY(int mode READ mode NOTIFY modeChanged)
        Q_PROPERTY(int tonality READ tonality NOTIFY tonalityChanged)
        Q_PROPERTY(int accidentalType READ accidentalType NOTIFY accidentalTypeChanged)
    public:
        explicit KeySignatureAtSpecifiedPositionHelper(QObject *parent = nullptr);
        ~KeySignatureAtSpecifiedPositionHelper() override;

        int position() const;
        void setPosition(int position);

        dspx::KeySignatureSequence *keySignatureSequence() const;
        void setKeySignatureSequence(dspx::KeySignatureSequence *keySignatureSequence);

        dspx::KeySignature *keySignature() const;
        dspx::KeySignature *keySignatureAt(int position) const;

        int mode() const;
        int tonality() const;
        int accidentalType() const;
        int accidentalTypeAt(int position) const;

    Q_SIGNALS:
        void positionChanged();
        void keySignatureSequenceChanged();
        void keySignatureChanged();
        void modeChanged();
        void tonalityChanged();
        void accidentalTypeChanged();
        void keySignatureLookupChanged();

    private:
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

        int m_position = 0;
        dspx::KeySignatureSequence *m_keySignatureSequence = nullptr;
        dspx::KeySignature *m_keySignature = nullptr;
        QMap<int, dspx::KeySignature *> m_keySignatureLookup;
        QHash<dspx::KeySignature *, int> m_keySignaturePositions;
    };

}

#endif //DIFFSCOPE_COREPLUGIN_KEYSIGNATUREATSPECIFIEDPOSITIONHELPER_H
