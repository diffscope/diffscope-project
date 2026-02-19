#ifndef DIFFSCOPE_DSPX_MODEL_SELECTIONMODEL_H
#define DIFFSCOPE_DSPX_MODEL_SELECTIONMODEL_H

#include <QObject>
#include <qqmlintegration.h>

#include <dspxmodel/DspxModelGlobal.h>

namespace dspx {

    class Model;

    class AnchorNodeSelectionModel;
    class ClipSelectionModel;
    class LabelSelectionModel;
    class NoteSelectionModel;
    class TempoSelectionModel;
    class TrackSelectionModel;
    class SelectionModelPrivate;

    class DSPX_MODEL_EXPORT SelectionModel : public QObject {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_DECLARE_PRIVATE(SelectionModel)
        Q_PROPERTY(Model *model READ model CONSTANT)
        Q_PROPERTY(SelectionType selectionType READ selectionType NOTIFY selectionTypeChanged)
        Q_PROPERTY(AnchorNodeSelectionModel *anchorNodeSelectionModel READ anchorNodeSelectionModel CONSTANT)
        Q_PROPERTY(ClipSelectionModel *clipSelectionModel READ clipSelectionModel CONSTANT)
        Q_PROPERTY(LabelSelectionModel *labelSelectionModel READ labelSelectionModel CONSTANT)
        Q_PROPERTY(NoteSelectionModel *noteSelectionModel READ noteSelectionModel CONSTANT)
        Q_PROPERTY(TempoSelectionModel *tempoSelectionModel READ tempoSelectionModel CONSTANT)
        Q_PROPERTY(TrackSelectionModel *trackSelectionModel READ trackSelectionModel CONSTANT)
        Q_PROPERTY(QObject *currentItemSelectionModel READ currentItemSelectionModel NOTIFY currentItemSelectionModelChanged)
        Q_PROPERTY(QObject *currentItem READ currentItem NOTIFY currentItemChanged)
        Q_PROPERTY(int selectedCount READ selectedCount NOTIFY selectedCountChanged)

    public:
        enum SelectionType {
            ST_None,
            ST_AnchorNode,
            ST_Clip,
            ST_Label,
            ST_Note,
            ST_Tempo,
            ST_Track
        };
        Q_ENUM(SelectionType)

        explicit SelectionModel(Model *model, QObject *parent = nullptr);
        ~SelectionModel() override;

        Model *model() const;

        SelectionType selectionType() const;

        AnchorNodeSelectionModel *anchorNodeSelectionModel() const;
        ClipSelectionModel *clipSelectionModel() const;
        LabelSelectionModel *labelSelectionModel() const;
        NoteSelectionModel *noteSelectionModel() const;
        TempoSelectionModel *tempoSelectionModel() const;
        TrackSelectionModel *trackSelectionModel() const;

        QObject *currentItemSelectionModel() const;
        QObject *currentItem() const;
        int selectedCount() const;
        Q_INVOKABLE bool isItemSelected(QObject *item) const;

        enum SelectionCommandFlag {
            Select = 0x1,
            Deselect = 0x2,
            Toggle = Select | Deselect,
            ClearPreviousSelection = 0x4,
            SetCurrentItem = 0x8,
        };
        Q_ENUM(SelectionCommandFlag)
        Q_DECLARE_FLAGS(SelectionCommand, SelectionCommandFlag)

        Q_INVOKABLE static SelectionType selectionTypeFromItem(QObject *item);
        Q_INVOKABLE void select(QObject *item, SelectionCommand command, SelectionType emptySelectionType = {}, QObject *containerItemHint = {});

    Q_SIGNALS:
        void selectionTypeChanged();
        void currentItemSelectionModelChanged();
        void currentItemChanged();
        void selectedCountChanged();
        void itemSelected(QObject *item, bool selected);

    private:
        QScopedPointer<SelectionModelPrivate> d_ptr;
    };

    Q_DECLARE_OPERATORS_FOR_FLAGS(SelectionModel::SelectionCommand)

}

#endif //DIFFSCOPE_DSPX_MODEL_SELECTIONMODEL_H