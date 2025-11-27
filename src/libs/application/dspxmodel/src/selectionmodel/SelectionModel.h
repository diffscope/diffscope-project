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
        Q_DECLARE_PRIVATE(SelectionModel)
        Q_PROPERTY(Model *model READ model CONSTANT)
        Q_PROPERTY(SelectionType selectionType READ selectionType NOTIFY selectionTypeChanged)
        Q_PROPERTY(AnchorNodeSelectionModel *anchorNodeSelectionModel READ anchorNodeSelectionModel CONSTANT)
        Q_PROPERTY(ClipSelectionModel *clipSelectionModel READ clipSelectionModel CONSTANT)
        Q_PROPERTY(LabelSelectionModel *labelSelectionModel READ labelSelectionModel CONSTANT)
        Q_PROPERTY(NoteSelectionModel *noteSelectionModel READ noteSelectionModel CONSTANT)
        Q_PROPERTY(TempoSelectionModel *tempoSelectionModel READ tempoSelectionModel CONSTANT)
        Q_PROPERTY(TrackSelectionModel *trackSelectionModel READ trackSelectionModel CONSTANT)

    public:
        enum SelectionType {
            None,
            AnchorNode,
            Clip,
            Label,
            Note,
            Tempo,
            Track
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

        enum SelectionCommandFlag {
            Select = 0x1,
            Deselect = 0x2,
            Toggle = Select | Deselect,
            ClearPreviousSelection = 0x4,
            SetCurrentItem = 0x8,
        };
        Q_ENUM(SelectionCommandFlag)
        Q_DECLARE_FLAGS(SelectionCommand, SelectionCommandFlag)

        Q_INVOKABLE void select(QObject *item, SelectionCommand command);

    Q_SIGNALS:
        void selectionTypeChanged();

    private:
        QScopedPointer<SelectionModelPrivate> d_ptr;
    };

    Q_DECLARE_OPERATORS_FOR_FLAGS(SelectionModel::SelectionCommand)

}

#endif //DIFFSCOPE_DSPX_MODEL_SELECTIONMODEL_H