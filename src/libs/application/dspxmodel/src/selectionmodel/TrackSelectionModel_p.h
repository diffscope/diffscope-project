#ifndef DIFFSCOPE_DSPX_MODEL_TRACKSELECTIONMODEL_P_H
#define DIFFSCOPE_DSPX_MODEL_TRACKSELECTIONMODEL_P_H

#include <dspxmodel/TrackSelectionModel.h>
#include <dspxmodel/private/GenericGlobalItemSelectionModelData_p.h>
#include <dspxmodel/Track.h>

namespace dspx {

    class TrackSelectionModelPrivate : public GenericGlobalItemSelectionModelData<
        TrackSelectionModel,
        TrackSelectionModelPrivate,
        Track,
        &Track::trackListChanged
    > {
        Q_DECLARE_PUBLIC(TrackSelectionModel)
    public:
        bool isAddedToModel(Track *item) const;
    };

}

#endif //DIFFSCOPE_DSPX_MODEL_TRACKSELECTIONMODEL_P_H