#ifndef DIFFSCOPE_VISUALEDITOR_KEYSIGNATURESELECTIONCONTROLLER_P_H
#define DIFFSCOPE_VISUALEDITOR_KEYSIGNATURESELECTIONCONTROLLER_P_H

#include <ScopicFlowCore/SelectionController.h>

class QQuickItem;

namespace dspx {
    class SelectionModel;
    class KeySignature;
    class KeySignatureSelectionModel;
    class KeySignatureSequence;
}

namespace VisualEditor {

    class ProjectViewModelContext;

    class KeySignatureSelectionController : public sflow::SelectionController {
        Q_OBJECT
    public:
        explicit KeySignatureSelectionController(ProjectViewModelContext *parent);
        ~KeySignatureSelectionController() override;

        QObjectList getSelectedItems() const override;
        QObjectList getItemsBetween(QObject *startItem, QObject *endItem) const override;
        void select(QObject *item, SelectionCommand command) override;
        QObject *currentItem() const override;
        bool editScopeFocused() const override;

    private:
        ProjectViewModelContext *q;
        dspx::KeySignatureSequence *keySignatureSequence;
        dspx::SelectionModel *selectionModel;
        dspx::KeySignatureSelectionModel *keySignatureSelectionModel;
    };

}

#endif // DIFFSCOPE_VISUALEDITOR_KEYSIGNATURESELECTIONCONTROLLER_P_H
