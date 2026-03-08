#ifndef DIFFSCOPE_DSPX_MODEL_KEYSIGNATURESELECTIONMODEL_H
#define DIFFSCOPE_DSPX_MODEL_KEYSIGNATURESELECTIONMODEL_H

#include <qqmlintegration.h>

#include <dspxmodel/SelectionModel.h>

namespace dspx {

    class SelectionModel;
    class KeySignature;
    class KeySignatureSelectionModelPrivate;

    class DSPX_MODEL_EXPORT KeySignatureSelectionModel : public QObject {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_DECLARE_PRIVATE(KeySignatureSelectionModel)

        Q_PROPERTY(KeySignature *currentItem READ currentItem NOTIFY currentItemChanged)
        Q_PROPERTY(QList<KeySignature *> selectedItems READ selectedItems NOTIFY selectedItemsChanged)
        Q_PROPERTY(int selectedCount READ selectedCount NOTIFY selectedCountChanged)

    public:
        ~KeySignatureSelectionModel() override;

        KeySignature *currentItem() const;
        QList<KeySignature *> selectedItems() const;
        int selectedCount() const;

        Q_INVOKABLE bool isItemSelected(KeySignature *item) const;

    Q_SIGNALS:
        void currentItemChanged();
        void selectedItemsChanged();
        void selectedCountChanged();
        void itemSelected(KeySignature *item, bool selected);

    private:
        friend class SelectionModel;
        explicit KeySignatureSelectionModel(SelectionModel *parent = nullptr);
        QScopedPointer<KeySignatureSelectionModelPrivate> d_ptr;
    };

}

#endif //DIFFSCOPE_DSPX_MODEL_KEYSIGNATURESELECTIONMODEL_H
