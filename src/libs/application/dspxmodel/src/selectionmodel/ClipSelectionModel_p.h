#ifndef DIFFSCOPE_DSPX_MODEL_CLIPSELECTIONMODEL_P_H
#define DIFFSCOPE_DSPX_MODEL_CLIPSELECTIONMODEL_P_H

#include <dspxmodel/ClipSelectionModel.h>
#include <dspxmodel/private/GenericGlobalItemSelectionModelData_p.h>
#include <dspxmodel/Clip.h>

#include <QHash>

namespace dspx {

    class ClipSelectionModelPrivate : public GenericGlobalItemSelectionModelData<
        ClipSelectionModel,
        ClipSelectionModelPrivate,
        Clip,
        &Clip::clipSequenceChanged
    > {
        Q_DECLARE_PUBLIC(ClipSelectionModel)
    public:
        QHash<Clip *, ClipSequence *> clipClipSequence;
        QHash<ClipSequence *, QSet<Clip *>> clipSequencesWithSelectedItems;

        bool isAddedToModel(Clip *item) const;
        void updateAssociation(Clip *item);
        void removeAssociation(Clip *item);
        void clearAssociation();

    };

}

#endif //DIFFSCOPE_DSPX_MODEL_CLIPSELECTIONMODEL_P_H