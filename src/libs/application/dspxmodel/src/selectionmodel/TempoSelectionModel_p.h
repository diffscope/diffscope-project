#ifndef DIFFSCOPE_DSPX_MODEL_TEMPOSELECTIONMODEL_P_H
#define DIFFSCOPE_DSPX_MODEL_TEMPOSELECTIONMODEL_P_H

#include <dspxmodel/TempoSelectionModel.h>
#include <dspxmodel/Tempo.h>
#include <dspxmodel/private/GenericGlobalItemSelectionModelData_p.h>

namespace dspx {

    class TempoSelectionModelPrivate : public GenericGlobalItemSelectionModelData<
        TempoSelectionModel,
        TempoSelectionModelPrivate,
        Tempo,
        &Tempo::tempoSequenceChanged
    > {
        Q_DECLARE_PUBLIC(TempoSelectionModel)
    public:
        bool isAddedToModel(Tempo *item) const;
    };

}

#endif //DIFFSCOPE_DSPX_MODEL_TEMPOSELECTIONMODEL_P_H