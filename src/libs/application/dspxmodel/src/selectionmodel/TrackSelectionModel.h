#ifndef DIFFSCOPE_DSPX_MODEL_TRACKSELECTIONMODEL_H
#define DIFFSCOPE_DSPX_MODEL_TRACKSELECTIONMODEL_H

#include <qqmlintegration.h>

#include <dspxmodel/SelectionModel.h>

namespace dspx {

    class Track;
    class TrackSelectionModelPrivate;

    class DSPX_MODEL_EXPORT TrackSelectionModel : public QObject {
        Q_OBJECT
        QML_ELEMENT
        Q_DECLARE_PRIVATE(TrackSelectionModel)

        Q_PROPERTY(Track *currentItem READ currentItem NOTIFY currentItemChanged)
        Q_PROPERTY(QList<Track *> selectedItems READ selectedItems NOTIFY selectedItemsChanged)
        Q_PROPERTY(int selectedCount READ selectedCount NOTIFY selectedCountChanged)

    public:
        explicit TrackSelectionModel(QObject *parent = nullptr);
        ~TrackSelectionModel() override;

        Track *currentItem() const;
        QList<Track *> selectedItems() const;
        int selectedCount() const;

        Q_INVOKABLE void select(Track *item, SelectionModel::SelectionCommand command);

    Q_SIGNALS:
        void currentItemChanged();
        void selectedItemsChanged();
        void selectedCountChanged();

    private:
        QScopedPointer<TrackSelectionModelPrivate> d_ptr;
    };

}

#endif //DIFFSCOPE_DSPX_MODEL_TRACKSELECTIONMODEL_H