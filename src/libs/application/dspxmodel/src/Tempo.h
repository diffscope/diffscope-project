#ifndef DIFFSCOPE_DSPX_MODEL_TEMPO_H
#define DIFFSCOPE_DSPX_MODEL_TEMPO_H

#include <dspxmodel/EntityObject.h>
#include <qqmlintegration.h>

namespace QDspx {
    struct Tempo;
}

namespace dspx {

    class TempoPrivate;

    class DSPX_MODEL_EXPORT Tempo : public EntityObject {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_DECLARE_PRIVATE(Tempo);
        Q_PRIVATE_PROPERTY(d_func(), int pos MEMBER pos WRITE setPos NOTIFY posChanged)
        Q_PRIVATE_PROPERTY(d_func(), double value MEMBER value WRITE setValue NOTIFY valueChanged)
    public:
        ~Tempo() override;

        int pos() const;
        void setPos(int pos);

        double value() const;
        void setValue(double value);

        QDspx::Tempo toQDspx() const;
        void fromQDspx(const QDspx::Tempo &tempo);

    Q_SIGNALS:
        void posChanged(int pos);
        void valueChanged(double value);

    protected:
        void handleSetEntityProperty(int property, const QVariant &value) override;

    private:
        friend class ModelPrivate;
        explicit Tempo(Handle handle, Model *model);
        QScopedPointer<TempoPrivate> d_ptr;

    };

}

#endif //DIFFSCOPE_DSPX_MODEL_TEMPO_H