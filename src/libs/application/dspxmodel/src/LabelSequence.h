#ifndef DIFFSCOPE_DSPX_MODEL_LABELSEQUENCE_H
#define DIFFSCOPE_DSPX_MODEL_LABELSEQUENCE_H

#include <qqmlintegration.h>

#include <dspxmodel/EntityObject.h>

namespace dspx {

    class LabelSequencePrivate;

    class LabelSequence : public EntityObject {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_DECLARE_PRIVATE(LabelSequence)
    public:
        ~LabelSequence() override;

    protected:
        void handleInsertIntoSequenceContainer(Handle entity) override;
        void handleTakeFromSequenceContainer(Handle takenEntity, Handle entity) override;

    private:
        friend class ModelPrivate;
        explicit LabelSequence(Handle handle, Model *model);
        QScopedPointer<LabelSequencePrivate> d_ptr;
    };

}

#endif //DIFFSCOPE_DSPX_MODEL_LABELSEQUENCE_H
