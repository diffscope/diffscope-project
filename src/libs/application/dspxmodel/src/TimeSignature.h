#ifndef DIFFSCOPE_DSPX_MODEL_TIMESIGNATURE_H
#define DIFFSCOPE_DSPX_MODEL_TIMESIGNATURE_H

#include <qqmlintegration.h>

#include <dspxmodel/EntityObject.h>

namespace QDspx {
    struct TimeSignature;
}

namespace dspx {

    class TimeSignatureSequence;
    class TimeSignaturePrivate;

    class DSPX_MODEL_EXPORT TimeSignature : public EntityObject {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_DECLARE_PRIVATE(TimeSignature);
        Q_PROPERTY(int index READ index WRITE setIndex NOTIFY indexChanged)
        Q_PROPERTY(int numerator READ numerator WRITE setNumerator NOTIFY numeratorChanged)
        Q_PROPERTY(int denominator READ denominator WRITE setDenominator NOTIFY denominatorChanged)
        Q_PROPERTY(TimeSignatureSequence *timeSignatureSequence READ timeSignatureSequence NOTIFY timeSignatureSequenceChanged)
    public:
        ~TimeSignature() override;

        int index() const;
        void setIndex(int index);

        int numerator() const;
        void setNumerator(int numerator);

        int denominator() const;
        void setDenominator(int denominator);

        TimeSignatureSequence *timeSignatureSequence() const;

        QDspx::TimeSignature toQDspx() const;
        void fromQDspx(const QDspx::TimeSignature &timeSignature);

    Q_SIGNALS:
        void indexChanged(int index);
        void numeratorChanged(int numerator);
        void denominatorChanged(int denominator);
        void timeSignatureSequenceChanged();

    protected:
        void handleSetEntityProperty(int property, const QVariant &value) override;

    private:
        friend class ModelPrivate;
        explicit TimeSignature(Handle handle, Model *model);
        QScopedPointer<TimeSignaturePrivate> d_ptr;
    };

}

#endif //DIFFSCOPE_DSPX_MODEL_TIMESIGNATURE_H
