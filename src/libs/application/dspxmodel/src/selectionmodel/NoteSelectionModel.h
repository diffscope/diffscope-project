#ifndef DIFFSCOPE_DSPX_MODEL_NOTESELECTIONMODEL_H
#define DIFFSCOPE_DSPX_MODEL_NOTESELECTIONMODEL_H

#include <QObject>
#include <QList>
#include <qqmlintegration.h>

#include <dspxmodel/DspxModelGlobal.h>

namespace dspx {

    class SelectionModel;
    class Note;
    class NoteSequence;
    class NoteSelectionModelPrivate;

    class DSPX_MODEL_EXPORT NoteSelectionModel : public QObject {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_DECLARE_PRIVATE(NoteSelectionModel)

        Q_PROPERTY(Note *currentItem READ currentItem NOTIFY currentItemChanged)
        Q_PROPERTY(QList<Note *> selectedItems READ selectedItems NOTIFY selectedItemsChanged)
        Q_PROPERTY(int selectedCount READ selectedCount NOTIFY selectedCountChanged)
        Q_PROPERTY(NoteSequence *noteSequenceWithSelectedItems READ noteSequenceWithSelectedItems NOTIFY noteSequenceWithSelectedItemsChanged)

    public:
        ~NoteSelectionModel() override;

        Note *currentItem() const;
        QList<Note *> selectedItems() const;
        int selectedCount() const;
        NoteSequence *noteSequenceWithSelectedItems() const;

        Q_INVOKABLE bool isItemSelected(Note *item) const;

    Q_SIGNALS:
        void currentItemChanged();
        void selectedItemsChanged();
        void selectedCountChanged();
        void noteSequenceWithSelectedItemsChanged();
        void itemSelected(Note *item, bool selected);

    private:
        friend class SelectionModel;
        explicit NoteSelectionModel(QObject *parent = nullptr);
        QScopedPointer<NoteSelectionModelPrivate> d_ptr;
    };

}

#endif //DIFFSCOPE_DSPX_MODEL_NOTESELECTIONMODEL_H