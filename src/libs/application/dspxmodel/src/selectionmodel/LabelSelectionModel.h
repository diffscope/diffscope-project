#ifndef DIFFSCOPE_DSPX_MODEL_LABELSELECTIONMODEL_H
#define DIFFSCOPE_DSPX_MODEL_LABELSELECTIONMODEL_H

#include <qqmlintegration.h>

#include <dspxmodel/SelectionModel.h>

namespace dspx {

    class SelectionModel;
    class Label;
    class LabelSelectionModelPrivate;

    class DSPX_MODEL_EXPORT LabelSelectionModel : public QObject {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_DECLARE_PRIVATE(LabelSelectionModel)

        Q_PROPERTY(Label *currentItem READ currentItem NOTIFY currentItemChanged)
        Q_PROPERTY(QList<Label *> selectedItems READ selectedItems NOTIFY selectedItemsChanged)
        Q_PROPERTY(int selectedCount READ selectedCount NOTIFY selectedCountChanged)

    public:
        ~LabelSelectionModel() override;

        Label *currentItem() const;
        QList<Label *> selectedItems() const;
        int selectedCount() const;

        Q_INVOKABLE bool isItemSelected(Label *item) const;

    Q_SIGNALS:
        void currentItemChanged();
        void selectedItemsChanged();
        void selectedCountChanged();
        void itemSelected(Label *item, bool selected);

    private:
        friend class SelectionModel;
        explicit LabelSelectionModel(SelectionModel *parent);
        QScopedPointer<LabelSelectionModelPrivate> d_ptr;
    };

}

#endif //DIFFSCOPE_DSPX_MODEL_LABELSELECTIONMODEL_H