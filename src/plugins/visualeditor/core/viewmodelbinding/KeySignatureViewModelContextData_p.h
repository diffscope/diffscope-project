#ifndef DIFFSCOPE_VISUALEDITOR_KEYSIGNATUREVIEWMODELCONTEXTDATA_P_H
#define DIFFSCOPE_VISUALEDITOR_KEYSIGNATUREVIEWMODELCONTEXTDATA_P_H

#include <QHash>
#include <QSet>

#include <transactional/TransactionController.h>
#include <visualeditor/ProjectViewModelContext.h>

class QStateMachine;
class QState;
class QQuickItem;

namespace dspx {
    class KeySignatureSequence;
    class KeySignatureSelectionModel;
    class SelectionModel;
}

namespace sflow {
    class LabelSequenceInteractionController;
    class LabelViewModel;
    class PointSequenceViewModel;
    class RangeSequenceViewModel;
    class ScaleHighlightViewModel;
}

namespace Core {
    class DspxDocument;
}

namespace VisualEditor {

    class KeySignatureSelectionController;

    class KeySignatureViewModelContextData : public QObject {
        Q_OBJECT
        Q_DECLARE_PUBLIC(ProjectViewModelContext)
    public:
        ProjectViewModelContext *q_ptr;

        // Cached document objects
        Core::DspxDocument *document;
        dspx::KeySignatureSequence *keySignatureSequence;
        dspx::KeySignatureSelectionModel *keySignatureSelectionModel;

        sflow::PointSequenceViewModel *keySignatureSequenceViewModel;
        KeySignatureSelectionController *keySignatureSelectionController;

        sflow::RangeSequenceViewModel *scaleHighlightSequenceViewModel;

        QHash<dspx::KeySignature *, sflow::LabelViewModel *> keySignatureViewItemMap;
        QHash<dspx::KeySignature *, sflow::ScaleHighlightViewModel *> keySignatureScaleHighlightViewItemMap;
        QHash<sflow::LabelViewModel *, dspx::KeySignature *> keySignatureDocumentItemMap;
        QSet<sflow::LabelViewModel *> transactionalUpdatedKeySignatures;

        QStateMachine *stateMachine;
        QState *idleState;
        QState *movePendingState;
        QState *moveProgressingState;
        QState *moveCommittingState;
        QState *moveAbortingState;
        QState *rubberBandDraggingState;

        Core::TransactionController::TransactionId moveTransactionId{};

        void init();
        void initStateMachine();
        void bindKeySignatureSequenceViewModel();
        void bindKeySignatureDocumentItem(dspx::KeySignature *item);
        void unbindKeySignatureDocumentItem(dspx::KeySignature *item);
        void bindScaleHighlightDocumentItem(dspx::KeySignature *item);
        void unbindScaleHighlightDocumentItem(dspx::KeySignature *item);
        void updateScaleHighlightViewItem(dspx::KeySignature *item);
        sflow::LabelSequenceInteractionController *createController(QObject *parent);

        void onMovePendingStateEntered();
        void onMoveCommittingStateEntered();
        void onMoveAbortingStateEntered();
        void onDoubleClicked(QQuickItem *labelSequenceItem, int position);
        void onItemDoubleClicked(QQuickItem *labelSequenceItem, sflow::LabelViewModel *viewItem);

    Q_SIGNALS:
        void moveTransactionWillStart();
        void moveTransactionStarted();
        void moveTransactionNotStarted();
        void moveTransactionWillCommit();
        void moveTransactionWillAbort();
        void rubberBandDragWillStart();
        void rubberBandDragWillFinish();
    };

}

#endif //DIFFSCOPE_VISUALEDITOR_KEYSIGNATUREVIEWMODELCONTEXTDATA_P_H
