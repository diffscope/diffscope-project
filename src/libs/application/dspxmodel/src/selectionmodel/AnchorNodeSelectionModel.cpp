#include "AnchorNodeSelectionModel.h"
#include "AnchorNodeSelectionModel_p.h"

#include <dspxmodel/AnchorNode.h>
#include <dspxmodel/ParamCurveAnchor.h>
#include <dspxmodel/ParamCurveSequence.h>

namespace dspx {

    AnchorNodeSelectionModel::AnchorNodeSelectionModel(QObject *parent) : QObject(parent), d_ptr(new AnchorNodeSelectionModelPrivate) {
        Q_D(AnchorNodeSelectionModel);
        d->q_ptr = this;
    }

    AnchorNodeSelectionModel::~AnchorNodeSelectionModel() = default;

    AnchorNode *AnchorNodeSelectionModel::currentItem() const {
        Q_D(const AnchorNodeSelectionModel);
        return d->currentItem;
    }

    QList<AnchorNode *> AnchorNodeSelectionModel::selectedItems() const {
        Q_D(const AnchorNodeSelectionModel);
        return d->selectedItems;
    }

    int AnchorNodeSelectionModel::selectedCount() const {
        Q_D(const AnchorNodeSelectionModel);
        return d->selectedItems.size();
    }

    QList<ParamCurveAnchor *> AnchorNodeSelectionModel::paramCurvesAnchorWithSelectedItems() const {
        Q_D(const AnchorNodeSelectionModel);
        return d->paramCurvesAnchorWithSelectedItems;
    }

    ParamCurveSequence *AnchorNodeSelectionModel::paramCurveSequenceWithSelectedItems() const {
        Q_D(const AnchorNodeSelectionModel);
        return d->paramCurveSequenceWithSelectedItems;
    }

}

#include "moc_AnchorNodeSelectionModel.cpp"
