#ifndef DIFFSCOPE_DSPX_MODEL_TEMPOSELECTIONMODEL_H
#define DIFFSCOPE_DSPX_MODEL_TEMPOSELECTIONMODEL_H

#include <qqmlintegration.h>

#include <dspxmodel/SelectionModel.h>

namespace dspx {

    class SelectionModel;
    class Tempo;
    class TempoSelectionModelPrivate;

    class DSPX_MODEL_EXPORT TempoSelectionModel : public QObject {
        Q_OBJECT
        QML_ELEMENT
        Q_DECLARE_PRIVATE(TempoSelectionModel)

        Q_PROPERTY(Tempo *currentItem READ currentItem NOTIFY currentItemChanged)
        Q_PROPERTY(QList<Tempo *> selectedItems READ selectedItems NOTIFY selectedItemsChanged)
        Q_PROPERTY(int selectedCount READ selectedCount NOTIFY selectedCountChanged)

    public:
        ~TempoSelectionModel() override;

        Tempo *currentItem() const;
        QList<Tempo *> selectedItems() const;
        int selectedCount() const;

        Q_INVOKABLE bool isItemSelected(Tempo *item) const;

    Q_SIGNALS:
        void currentItemChanged();
        void selectedItemsChanged();
        void selectedCountChanged();
        void itemSelected(Tempo *item, bool selected);

    private:
        friend class SelectionModel;
        explicit TempoSelectionModel(SelectionModel *parent = nullptr);
        QScopedPointer<TempoSelectionModelPrivate> d_ptr;
    };

}

#endif //DIFFSCOPE_DSPX_MODEL_TEMPOSELECTIONMODEL_H