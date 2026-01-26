#ifndef DIFFSCOPE_DSPX_MODEL_TEMPO_H
#define DIFFSCOPE_DSPX_MODEL_TEMPO_H

#include <qqmlintegration.h>

#include <dspxmodel/EntityObject.h>

namespace QDspx {
    struct Tempo;
}

namespace dspx {

    class TempoSequence;
    class TempoPrivate;

    class DSPX_MODEL_EXPORT Tempo : public EntityObject {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_DECLARE_PRIVATE(Tempo);
        Q_PROPERTY(int pos READ pos WRITE setPos NOTIFY posChanged)
        Q_PROPERTY(double value READ value WRITE setValue NOTIFY valueChanged)
        Q_PROPERTY(TempoSequence *tempoSequence READ tempoSequence NOTIFY tempoSequenceChanged)
    public:
        ~Tempo() override;

        int pos() const;
        void setPos(int pos);

        double value() const;
        void setValue(double value);

        TempoSequence *tempoSequence() const;

        QDspx::Tempo toQDspx() const;
        void fromQDspx(const QDspx::Tempo &tempo);

    Q_SIGNALS:
        void posChanged(int pos);
        void valueChanged(double value);
        void tempoSequenceChanged();

    protected:
        void handleSetEntityProperty(int property, const QVariant &value) override;

    private:
        friend class ModelPrivate;
        explicit Tempo(Handle handle, Model *model);
        QScopedPointer<TempoPrivate> d_ptr;
    };

}

#endif //DIFFSCOPE_DSPX_MODEL_TEMPO_H
