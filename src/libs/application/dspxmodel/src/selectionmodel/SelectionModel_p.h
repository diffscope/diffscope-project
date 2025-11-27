#ifndef DIFFSCOPE_DSPX_MODEL_SELECTIONMODEL_P_H
#define DIFFSCOPE_DSPX_MODEL_SELECTIONMODEL_P_H

#include <dspxmodel/SelectionModel.h>

namespace dspx {

    class SelectionModelPrivate {
        Q_DECLARE_PUBLIC(SelectionModel)
    public:
        SelectionModel *q_ptr;

        Model *model;

        SelectionModel::SelectionType selectionType = SelectionModel::None;
        AnchorNodeSelectionModel *anchorNodeSelectionModel;
        ClipSelectionModel *clipSelectionModel;
        LabelSelectionModel *labelSelectionModel;
        NoteSelectionModel *noteSelectionModel;
        TempoSelectionModel *tempoSelectionModel;
        TrackSelectionModel *trackSelectionModel;
    };

}

#endif //DIFFSCOPE_DSPX_MODEL_SELECTIONMODEL_P_H