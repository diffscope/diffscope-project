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

    /** Tracks the effective key signature at a project tick in an optional borrowed sequence. */
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
        /** Creates a helper at tick zero without a bound sequence. */
        explicit KeySignatureAtSpecifiedPositionHelper(QObject *parent = nullptr);
        /** Destroys the helper and disconnects the borrowed sequence. */
        ~KeySignatureAtSpecifiedPositionHelper() override;

        /** Returns the lookup position in project ticks. */
        int position() const;
        /** Sets the lookup position in project ticks. */
        void setPosition(int position);

        /** Returns the borrowed sequence, or nullptr if unbound. */
        dspx::KeySignatureSequence *keySignatureSequence() const;
        /**
         * Binds a sequence, or clears the binding if nullptr.
         * @pre A non-null sequence remains valid until replaced, cleared, or helper destruction.
         */
        void setKeySignatureSequence(dspx::KeySignatureSequence *keySignatureSequence);

        /** Returns the effective key signature at position(), or nullptr if none applies. */
        dspx::KeySignature *keySignature() const;
        /** Returns the effective key signature at the project tick, or nullptr if none applies. */
        dspx::KeySignature *keySignatureAt(int position) const;

        /** Returns the effective scale mask, or zero if no key signature applies. */
        int mode() const;
        /** Returns the effective pitch class, or zero if no key signature applies. */
        int tonality() const;
        /** Returns the effective accidental type, or zero if no key signature applies. */
        int accidentalType() const;
        /** Returns the accidental type at the project tick, or zero if no key signature applies. */
        int accidentalTypeAt(int position) const;

    Q_SIGNALS:
        /** Notifies that the lookup position changed. */
        void positionChanged();
        /** Notifies that the bound sequence changed. */
        void keySignatureSequenceChanged();
        /** Notifies that the effective key signature object changed. */
        void keySignatureChanged();
        /** Notifies that the effective mode changed. */
        void modeChanged();
        /** Notifies that the effective tonality changed. */
        void tonalityChanged();
        /** Notifies that the effective accidental type changed. */
        void accidentalTypeChanged();
        /** Notifies that key signature lookup results may have changed. */
        void keySignatureLookupChanged();

    private:
        QScopedPointer<KeySignatureAtSpecifiedPositionHelperPrivate> d_ptr;
    };

}

#endif // DIFFSCOPE_DSPXPROJECT_KEYSIGNATUREATSPECIFIEDPOSITIONHELPER_H
