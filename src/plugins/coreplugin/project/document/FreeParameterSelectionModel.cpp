#include "FreeParameterSelectionModel.h"
#include "FreeParameterSelectionModel_p.h"

#include <algorithm>

#include <dspxmodelORM/SingingClip.h>
#include <dspxmodelSelectionModel/SelectionModel.h>

namespace Core {

    FreeParameterSelectionModel::FreeParameterSelectionModel(dspx::SelectionModel *selectionModel, QObject *parent)
        : QObject(parent), d_ptr(new FreeParameterSelectionModelPrivate) {
        Q_D(FreeParameterSelectionModel);
        d->q_ptr = this;
        d->selectionModel = selectionModel;
        connect(selectionModel, &dspx::SelectionModel::selectionTypeChanged, this, [this] {
            Q_D(FreeParameterSelectionModel);
            if (d->selectionModel->selectionType() != dspx::SelectionModel::ST_None) {
                clearContext();
            }
        });
    }

    FreeParameterSelectionModel::~FreeParameterSelectionModel() = default;

    dspx::SingingClip *FreeParameterSelectionModel::singingClip() const {
        Q_D(const FreeParameterSelectionModel);
        return d->singingClip;
    }

    QString FreeParameterSelectionModel::parameterId() const {
        Q_D(const FreeParameterSelectionModel);
        return d->parameterId;
    }

    QString FreeParameterSelectionModel::displayName() const {
        Q_D(const FreeParameterSelectionModel);
        return d->displayName;
    }

    FreeParameterSelectionModel::Layer FreeParameterSelectionModel::layer() const {
        Q_D(const FreeParameterSelectionModel);
        return d->layer;
    }

    bool FreeParameterSelectionModel::isActive() const {
        Q_D(const FreeParameterSelectionModel);
        return d->singingClip && !d->parameterId.isEmpty();
    }

    bool FreeParameterSelectionModel::hasSelection() const {
        Q_D(const FreeParameterSelectionModel);
        return d->hasSelection;
    }

    int FreeParameterSelectionModel::start() const {
        Q_D(const FreeParameterSelectionModel);
        return d->start;
    }

    int FreeParameterSelectionModel::end() const {
        Q_D(const FreeParameterSelectionModel);
        return d->end;
    }

    void FreeParameterSelectionModel::setContext(dspx::SingingClip *singingClip, const QString &parameterId,
                                                 const QString &displayName, Layer layer) {
        Q_D(FreeParameterSelectionModel);
        if (!singingClip || parameterId.isEmpty()) {
            clearContext();
            return;
        }
        const bool wasActive = isActive();
        const QString effectiveDisplayName = displayName.isEmpty() ? parameterId : displayName;
        const bool changed = d->singingClip != singingClip || d->parameterId != parameterId ||
                             d->displayName != effectiveDisplayName || d->layer != layer;
        if (changed) {
            clear();
            disconnect(d->singingClipDestroyedConnection);
            d->singingClip = singingClip;
            d->parameterId = parameterId;
            d->displayName = effectiveDisplayName;
            d->layer = layer;
            d->singingClipDestroyedConnection = connect(singingClip, &QObject::destroyed, this, [this] {
                clearContext();
            });
            Q_EMIT contextChanged();
        }
        d->selectionModel->select(nullptr, dspx::SelectionModel::ClearPreviousSelection,
                                  dspx::SelectionModel::ST_None);
        if (wasActive != isActive()) {
            Q_EMIT activeChanged();
        }
    }

    void FreeParameterSelectionModel::clearContext() {
        Q_D(FreeParameterSelectionModel);
        const bool wasActive = isActive();
        const bool changed = d->singingClip || !d->parameterId.isEmpty() || !d->displayName.isEmpty();
        clear();
        disconnect(d->singingClipDestroyedConnection);
        d->singingClipDestroyedConnection = {};
        d->singingClip = nullptr;
        d->parameterId.clear();
        d->displayName.clear();
        d->layer = EditedLayer;
        if (changed) {
            Q_EMIT contextChanged();
        }
        if (wasActive) {
            Q_EMIT activeChanged();
        }
    }

    void FreeParameterSelectionModel::setRange(int start, int end) {
        Q_D(FreeParameterSelectionModel);
        if (!isActive()) {
            return;
        }
        start = std::max(0, start);
        end = std::max(0, end);
        if (start > end) {
            std::swap(start, end);
        }
        const bool hasSelection = start < end;
        if (d->start == start && d->end == end && d->hasSelection == hasSelection) {
            return;
        }
        const bool selectionStateChanged = d->hasSelection != hasSelection;
        d->start = start;
        d->end = end;
        d->hasSelection = hasSelection;
        Q_EMIT rangeChanged();
        if (selectionStateChanged) {
            Q_EMIT hasSelectionChanged();
        }
    }

    void FreeParameterSelectionModel::clear() {
        Q_D(FreeParameterSelectionModel);
        if (d->start == 0 && d->end == 0 && !d->hasSelection) {
            return;
        }
        const bool hadSelection = d->hasSelection;
        d->start = 0;
        d->end = 0;
        d->hasSelection = false;
        Q_EMIT rangeChanged();
        if (hadSelection) {
            Q_EMIT hasSelectionChanged();
        }
    }
}

#include "moc_FreeParameterSelectionModel.cpp"
