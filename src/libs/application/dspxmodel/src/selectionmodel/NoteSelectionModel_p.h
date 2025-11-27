#ifndef DIFFSCOPE_DSPX_MODEL_NOTESELECTIONMODEL_P_H
#define DIFFSCOPE_DSPX_MODEL_NOTESELECTIONMODEL_P_H

#include <dspxmodel/NoteSelectionModel.h>

namespace dspx {

    class NoteSelectionModelPrivate {
        Q_DECLARE_PUBLIC(NoteSelectionModel)
    public:
        NoteSelectionModel *q_ptr;
        Note *currentItem = nullptr;
        QList<Note *> selectedItems;
        SingingClip *singingClipWithSelectedItems = nullptr;
    };

}

#endif //DIFFSCOPE_DSPX_MODEL_NOTESELECTIONMODEL_P_H