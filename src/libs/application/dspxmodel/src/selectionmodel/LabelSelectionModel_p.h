#ifndef DIFFSCOPE_DSPX_MODEL_LABELSELECTIONMODEL_P_H
#define DIFFSCOPE_DSPX_MODEL_LABELSELECTIONMODEL_P_H

#include <dspxmodel/LabelSelectionModel.h>
#include <dspxmodel/Label.h>
#include <dspxmodel/private/GenericGlobalItemSelectionModelData_p.h>

namespace dspx {

    class SelectionModel;

    class LabelSelectionModelPrivate : public GenericGlobalItemSelectionModelData<
        LabelSelectionModel,
        LabelSelectionModelPrivate,
        Label,
        &Label::labelSequenceChanged
    > {
        Q_DECLARE_PUBLIC(LabelSelectionModel)
    public:
        bool isAddedToModel(Label *item) const;
    };

}

#endif //DIFFSCOPE_DSPX_MODEL_LABELSELECTIONMODEL_P_H