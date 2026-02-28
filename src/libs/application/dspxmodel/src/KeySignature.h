#ifndef DIFFSCOPE_DSPX_MODEL_KEYSIGNATURE_H
#define DIFFSCOPE_DSPX_MODEL_KEYSIGNATURE_H

#include <qqmlintegration.h>

#include <dspxmodel/EntityObject.h>

namespace QDspx {
    struct KeySignature;
}

namespace dspx {

    class KeySignatureSequence;
    class KeySignaturePrivate;

    class DSPX_MODEL_EXPORT KeySignature : public EntityObject {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_DECLARE_PRIVATE(KeySignature);
        Q_PROPERTY(int pos READ pos WRITE setPos NOTIFY posChanged)
        Q_PROPERTY(int mode READ mode WRITE setMode NOTIFY modeChanged)
        Q_PROPERTY(int tonality READ tonality WRITE setTonality NOTIFY tonalityChanged)
        Q_PROPERTY(AccidentalType accidentalType READ accidentalType WRITE setAccidentalType NOTIFY accidentalTypeChanged)
    public:
        enum AccidentalType {
            Flat = 0,
            Sharp = 1
        };
        Q_ENUM(AccidentalType)

        ~KeySignature() override;

        int pos() const;
        void setPos(int pos);

        int mode() const;
        void setMode(int mode);

        int tonality() const;
        void setTonality(int tonality);

        AccidentalType accidentalType() const;
        void setAccidentalType(AccidentalType accidentalType);

        KeySignatureSequence *keySignatureSequence() const;

        KeySignature *previousItem() const;
        KeySignature *nextItem() const;

        QJsonObject toQDspx() const;
        void fromQDspx(const QJsonObject &keySignature);

    Q_SIGNALS:
        void posChanged(int pos);
        void modeChanged(int mode);
        void tonalityChanged(int tonality);
        void accidentalTypeChanged(AccidentalType accidentalType);
        void keySignatureSequenceChanged();
        void previousItemChanged();
        void nextItemChanged();

    protected:
        void handleSetEntityProperty(int property, const QVariant &value) override;

    private:
        friend class ModelPrivate;
        explicit KeySignature(Handle handle, Model *model);
        QScopedPointer<KeySignaturePrivate> d_ptr;
    };

}

#endif //DIFFSCOPE_DSPX_MODEL_KEYSIGNATURE_H
