// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_DSPXPROJECT_KEYSIGNATUREATSPECIFIEDPOSITIONHELPER_H
#define DIFFSCOPE_DSPXPROJECT_KEYSIGNATUREATSPECIFIEDPOSITIONHELPER_H

#include <QObject>
#include <QScopedPointer>
#include <qqmlintegration.h>

#include <dspxproject/dspxprojectglobal.h>

namespace dspx {
    class KeySignature;
    class KeySignatureSequence;
}

namespace DspxProject {

    class KeySignatureAtSpecifiedPositionHelperPrivate;

    class DSPXPROJECT_EXPORT KeySignatureAtSpecifiedPositionHelper : public QObject {
        Q_OBJECT
        QML_ELEMENT
        Q_DECLARE_PRIVATE(KeySignatureAtSpecifiedPositionHelper)
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
        // The sequence must remain valid until replaced, cleared, or the helper is destroyed.
        void setKeySignatureSequence(dspx::KeySignatureSequence *keySignatureSequence);

        dspx::KeySignature *keySignature() const;
        // Returns the effective key signature at the tick, or nullptr if none applies.
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
        QScopedPointer<KeySignatureAtSpecifiedPositionHelperPrivate> d_ptr;
    };

}

#endif // DIFFSCOPE_DSPXPROJECT_KEYSIGNATUREATSPECIFIEDPOSITIONHELPER_H
