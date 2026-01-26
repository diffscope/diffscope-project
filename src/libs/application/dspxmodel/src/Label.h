#ifndef DIFFSCOPE_DSPX_MODEL_LABEL_H
#define DIFFSCOPE_DSPX_MODEL_LABEL_H

#include <qqmlintegration.h>

#include <dspxmodel/EntityObject.h>

namespace QDspx {
    struct Label;
}

namespace dspx {

    class LabelSequence;
    class LabelPrivate;

    class DSPX_MODEL_EXPORT Label : public EntityObject {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_DECLARE_PRIVATE(Label);
        Q_PROPERTY(int pos READ pos WRITE setPos NOTIFY posChanged)
        Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged)
        Q_PROPERTY(LabelSequence *labelSequence READ labelSequence NOTIFY labelSequenceChanged)
    public:
        ~Label() override;

        int pos() const;
        void setPos(int pos);

        QString text() const;
        void setText(const QString &text);

        LabelSequence *labelSequence() const;

        QDspx::Label toQDspx() const;
        void fromQDspx(const QDspx::Label &label);

    Q_SIGNALS:
        void posChanged(int pos);
        void textChanged(const QString &text);
        void labelSequenceChanged();

    protected:
        void handleSetEntityProperty(int property, const QVariant &value) override;

    private:
        friend class ModelPrivate;
        explicit Label(Handle handle, Model *model);
        QScopedPointer<LabelPrivate> d_ptr;
    };

}

#endif //DIFFSCOPE_DSPX_MODEL_LABEL_H
