#ifndef DIFFSCOPE_DSPX_MODEL_ANCHORNODESELECTIONMODEL_H
#define DIFFSCOPE_DSPX_MODEL_ANCHORNODESELECTIONMODEL_H

#include <QObject>
#include <QList>
#include <qqmlintegration.h>

#include <dspxmodel/DspxModelGlobal.h>

namespace dspx {

    class AnchorNode;
    class ParamCurveAnchor;
    class ParamCurveSequence;
    class AnchorNodeSelectionModelPrivate;
    class SelectionModel;

    class DSPX_MODEL_EXPORT AnchorNodeSelectionModel : public QObject {
        Q_OBJECT
        QML_ELEMENT
        Q_DECLARE_PRIVATE(AnchorNodeSelectionModel)

        Q_PROPERTY(AnchorNode *currentItem READ currentItem NOTIFY currentItemChanged)
        Q_PROPERTY(QList<AnchorNode *> selectedItems READ selectedItems NOTIFY selectedItemsChanged)
        Q_PROPERTY(int selectedCount READ selectedCount NOTIFY selectedCountChanged)
        Q_PROPERTY(QList<ParamCurveAnchor *> paramCurvesAnchorWithSelectedItems READ paramCurvesAnchorWithSelectedItems NOTIFY paramCurvesAnchorWithSelectedItemsChanged)
        Q_PROPERTY(ParamCurveSequence *paramCurveSequenceWithSelectedItems READ paramCurveSequenceWithSelectedItems NOTIFY paramCurveSequenceWithSelectedItemsChanged)

    public:
        ~AnchorNodeSelectionModel() override;

        AnchorNode *currentItem() const;
        QList<AnchorNode *> selectedItems() const;
        int selectedCount() const;
        QList<ParamCurveAnchor *> paramCurvesAnchorWithSelectedItems() const;
        ParamCurveSequence *paramCurveSequenceWithSelectedItems() const;

    Q_SIGNALS:
        void currentItemChanged();
        void selectedItemsChanged();
        void selectedCountChanged();
        void paramCurvesAnchorWithSelectedItemsChanged();
        void paramCurveSequenceWithSelectedItemsChanged();

    private:
        friend class SelectionModel;
        explicit AnchorNodeSelectionModel(QObject *parent = nullptr);
        QScopedPointer<AnchorNodeSelectionModelPrivate> d_ptr;
    };

}

#endif //DIFFSCOPE_DSPX_MODEL_ANCHORNODESELECTIONMODEL_H