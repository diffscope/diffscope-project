#ifndef DIFFSCOPE_DSPX_MODEL_NOTESELECTIONMODEL_P_H
#define DIFFSCOPE_DSPX_MODEL_NOTESELECTIONMODEL_P_H

#include <dspxmodel/NoteSelectionModel.h>
#include <dspxmodel/private/GenericGlobalItemSelectionModelData_p.h>
#include <dspxmodel/Note.h>

namespace dspx {

    class NoteSelectionModelPrivate : public GenericGlobalItemSelectionModelData<
        NoteSelectionModel,
        NoteSelectionModelPrivate,
        Note,
        &Note::noteSequenceChanged,
        NoteSequence
    > {
        Q_DECLARE_PUBLIC(NoteSelectionModel)
    public:
        bool isAddedToModel(Note *item) const;

        static NoteSequence *getSuperItem(Note *item) {
            return item ? item->noteSequence() : nullptr;
        }

        void clearSuperItem();
        void updateSuperItem(NoteSequence *superItem);
    };

}

#endif //DIFFSCOPE_DSPX_MODEL_NOTESELECTIONMODEL_P_H