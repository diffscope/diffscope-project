#ifndef DIFFSCOPE_DSPX_MODEL_CLIPSELECTIONMODEL_H
#define DIFFSCOPE_DSPX_MODEL_CLIPSELECTIONMODEL_H

#include <qqmlintegration.h>

#include <dspxmodel/SelectionModel.h>

namespace dspx {

    class SelectionModel;
    class Clip;
    class ClipSequence;
    class ClipSelectionModelPrivate;

    class DSPX_MODEL_EXPORT ClipSelectionModel : public QObject {
        Q_OBJECT
        QML_ELEMENT
        Q_DECLARE_PRIVATE(ClipSelectionModel)

        Q_PROPERTY(Clip *currentItem READ currentItem NOTIFY currentItemChanged)
        Q_PROPERTY(QList<Clip *> selectedItems READ selectedItems NOTIFY selectedItemsChanged)
        Q_PROPERTY(int selectedCount READ selectedCount NOTIFY selectedCountChanged)
        Q_PROPERTY(QList<ClipSequence *> clipSequencesWithSelectedItems READ clipSequencesWithSelectedItems NOTIFY clipSequencesWithSelectedItemsChanged)

    public:
        ~ClipSelectionModel() override;

        Clip *currentItem() const;
        QList<Clip *> selectedItems() const;
        int selectedCount() const;
        QList<ClipSequence *> clipSequencesWithSelectedItems() const;

        Q_INVOKABLE bool isItemSelected(Clip *item) const;

    Q_SIGNALS:
        void currentItemChanged();
        void selectedItemsChanged();
        void selectedCountChanged();
        void clipSequencesWithSelectedItemsChanged();
        void itemSelected(Clip *item, bool selected);

    private:
        friend class SelectionModel;
        explicit ClipSelectionModel(SelectionModel *parent = nullptr);
        QScopedPointer<ClipSelectionModelPrivate> d_ptr;
    };

}

#endif //DIFFSCOPE_DSPX_MODEL_CLIPSELECTIONMODEL_H