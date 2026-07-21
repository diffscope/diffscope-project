#ifndef DIFFSCOPE_VISUALEDITOR_DYNAMICMIXINGEDITORCONTEXT_P_H
#define DIFFSCOPE_VISUALEDITOR_DYNAMICMIXINGEDITORCONTEXT_P_H

#include <QHash>
#include <QList>
#include <QMetaObject>
#include <QSet>

#include <ScopicFlowCore/SelectionController.h>

#include <transactional/TransactionController.h>

#include <visualeditor/DynamicMixingEditorContext.h>

class QState;
class QStateMachine;

namespace Core {
    class DspxDocument;
}

namespace dspx {
    class DynamicMixingAnchorSelectionModel;
    class DynamicMixingAnchorSequence;
    class SelectionModel;
    class Sources;
}

namespace VisualEditor {

    class DynamicMixingSelectionController;

    class DynamicMixingEditorContextPrivate : public QObject {
        Q_OBJECT
        Q_DECLARE_PUBLIC(DynamicMixingEditorContext)
    public:
        enum Operation {
            Idle,
            Inserting,
            Moving,
            Deleting,
            RubberBandSelecting,
        };

        DynamicMixingEditorContext *q_ptr{};
        ProjectViewModelContext *projectContext{};
        Core::DspxDocument *document{};
        dspx::SelectionModel *selectionModel{};
        dspx::DynamicMixingAnchorSelectionModel *dynamicSelectionModel{};
        dspx::SingingClip *singingClip{};
        dspx::Sources *sources{};
        dspx::DynamicMixingAnchorSequence *sequence{};

        sflow::DynamicMixingViewModel *viewModel{};
        DynamicMixingSelectionController *viewSelectionController{};
        sflow::DynamicMixingEditorInteractionController *interactionController{};

        QHash<dspx::DynamicMixingAnchor *, sflow::DynamicMixingAnchorViewModel *> viewItemMap;
        QHash<sflow::DynamicMixingAnchorViewModel *, dspx::DynamicMixingAnchor *> documentItemMap;
        QList<QMetaObject::Connection> targetConnections;
        QList<sflow::DynamicMixingAnchorViewModel *> operationItems;
        QList<dspx::DynamicMixingAnchor *> deletionItems;
        QList<dspx::DynamicMixingAnchor *> selectionBeforeOperation;
        dspx::DynamicMixingAnchor *currentItemBeforeOperation{};
        bool restoreSelectionOnAbort = false;
        Core::TransactionController::TransactionId transactionId{};
        Operation operation = Idle;
        bool suppressDocumentSync = false;
        bool rebuildPending = false;
        bool available = false;

        QStateMachine *stateMachine{};
        QState *idleState{};
        QState *insertionState{};
        QState *moveState{};
        QState *deletionState{};
        QState *rubberBandState{};

        void initialize();
        void disconnectTarget();
        void bindTarget();
        void rebuild();
        void clearViewItems();
        void bindDocumentItem(dspx::DynamicMixingAnchor *item,
                              sflow::DynamicMixingAnchorViewModel *existingViewItem = nullptr);
        void unbindDocumentItem(dspx::DynamicMixingAnchor *item);
        void updateVoiceCount();
        void syncSelection();
        void restoreOperationSelection();
        void finishOperation();
        bool beginOperation(Operation nextOperation);
        void abortOperation();
        void commitInsertion(sflow::DynamicMixingAnchorViewModel *viewItem);
        void commitMove();
        void commitDeletion();

    Q_SIGNALS:
        void insertionStarted();
        void moveStarted();
        void deletionStarted();
        void rubberBandStarted();
        void operationFinished();
    };

    class DynamicMixingSelectionController final : public sflow::SelectionController {
        Q_OBJECT
    public:
        explicit DynamicMixingSelectionController(DynamicMixingEditorContext *context);

        QObjectList getSelectedItems() const override;
        QObjectList getItemsBetween(QObject *startItem, QObject *endItem) const override;
        void select(QObject *item, SelectionCommand command) override;
        QObject *currentItem() const override;
        bool editScopeFocused() const override;

    private:
        DynamicMixingEditorContext *context;
    };

}

#endif // DIFFSCOPE_VISUALEDITOR_DYNAMICMIXINGEDITORCONTEXT_P_H
