#ifndef DIFFSCOPE_DSPX_MODEL_CLIPSELECTIONMODEL_H
#define DIFFSCOPE_DSPX_MODEL_CLIPSELECTIONMODEL_H

#include <QObject>
#include <QList>
#include <qqmlintegration.h>

#include <dspxmodel/DspxModelGlobal.h>

namespace dspx {

    class Clip;
    class Track;
    class ClipSelectionModelPrivate;

    class DSPX_MODEL_EXPORT ClipSelectionModel : public QObject {
        Q_OBJECT
        QML_ELEMENT
        Q_DECLARE_PRIVATE(ClipSelectionModel)

        Q_PROPERTY(Clip *currentItem READ currentItem NOTIFY currentItemChanged)
        Q_PROPERTY(QList<Clip *> selectedItems READ selectedItems NOTIFY selectedItemsChanged)
        Q_PROPERTY(int selectedCount READ selectedCount NOTIFY selectedCountChanged)
        Q_PROPERTY(QList<Track *> tracksWithSelectedItems READ tracksWithSelectedItems NOTIFY tracksWithSelectedItemsChanged)

    public:
        explicit ClipSelectionModel(QObject *parent = nullptr);
        ~ClipSelectionModel() override;

        Clip *currentItem() const;
        QList<Clip *> selectedItems() const;
        int selectedCount() const;
        QList<Track *> tracksWithSelectedItems() const;

    Q_SIGNALS:
        void currentItemChanged();
        void selectedItemsChanged();
        void selectedCountChanged();
        void tracksWithSelectedItemsChanged();

    private:
        QScopedPointer<ClipSelectionModelPrivate> d_ptr;
    };

}

#endif //DIFFSCOPE_DSPX_MODEL_CLIPSELECTIONMODEL_H