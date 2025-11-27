#ifndef DIFFSCOPE_DSPX_MODEL_NOTESELECTIONMODEL_H
#define DIFFSCOPE_DSPX_MODEL_NOTESELECTIONMODEL_H

#include <QObject>
#include <QList>
#include <qqmlintegration.h>

#include <dspxmodel/DspxModelGlobal.h>

namespace dspx {

    class Note;
    class SingingClip;
    class NoteSelectionModelPrivate;

    class DSPX_MODEL_EXPORT NoteSelectionModel : public QObject {
        Q_OBJECT
        QML_ELEMENT
        Q_DECLARE_PRIVATE(NoteSelectionModel)

        Q_PROPERTY(Note *currentItem READ currentItem NOTIFY currentItemChanged)
        Q_PROPERTY(QList<Note *> selectedItems READ selectedItems NOTIFY selectedItemsChanged)
        Q_PROPERTY(int selectedCount READ selectedCount NOTIFY selectedCountChanged)
        Q_PROPERTY(SingingClip *singingClipWithSelectedItems READ singingClipWithSelectedItems NOTIFY singingClipWithSelectedItemsChanged)

    public:
        explicit NoteSelectionModel(QObject *parent = nullptr);
        ~NoteSelectionModel() override;

        Note *currentItem() const;
        QList<Note *> selectedItems() const;
        int selectedCount() const;
        SingingClip *singingClipWithSelectedItems() const;

    Q_SIGNALS:
        void currentItemChanged();
        void selectedItemsChanged();
        void selectedCountChanged();
        void singingClipWithSelectedItemsChanged();

    private:
        QScopedPointer<NoteSelectionModelPrivate> d_ptr;
    };

}

#endif //DIFFSCOPE_DSPX_MODEL_NOTESELECTIONMODEL_H