#ifndef DIFFSCOPE_COREPLUGIN_KEYSIGNATUREATSPECIFIEDPOSITIONHELPER_H
#define DIFFSCOPE_COREPLUGIN_KEYSIGNATUREATSPECIFIEDPOSITIONHELPER_H

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

        int mode() const;
        int tonality() const;
        int accidentalType() const;

    Q_SIGNALS:
        void positionChanged();
        void keySignatureSequenceChanged();
        void keySignatureChanged();
        void modeChanged();
        void tonalityChanged();
        void accidentalTypeChanged();

    private:
        void updateKeySignature();
        void disconnectSequence();
        void connectSequence();
        void disconnectKeySignature();
        void connectKeySignature();

        int m_position = 0;
        dspx::KeySignatureSequence *m_keySignatureSequence = nullptr;
        dspx::KeySignature *m_keySignature = nullptr;
    };

}

#endif //DIFFSCOPE_COREPLUGIN_KEYSIGNATUREATSPECIFIEDPOSITIONHELPER_H
