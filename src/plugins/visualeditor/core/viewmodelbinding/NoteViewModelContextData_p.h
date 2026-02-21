#ifndef DIFFSCOPE_VISUALEDITOR_NOTEVIEWMODELCONTEXTDATA_P_H
#define DIFFSCOPE_VISUALEDITOR_NOTEVIEWMODELCONTEXTDATA_P_H

#include <QHash>
#include <QSet>

#include <transactional/TransactionController.h>
#include <visualeditor/ProjectViewModelContext.h>

class QState;
class QStateMachine;
class QQuickItem;

namespace dspx {
    class Note;
    class NoteSelectionModel;
    class SelectionModel;
    class SingingClip;
    class Track;
    class TrackList;
}

namespace sflow {
    class NoteEditLayerInteractionController;
    class NoteViewModel;
    class RangeSequenceViewModel;
}

namespace Core {
    class DspxDocument;
}

namespace VisualEditor {

    class NoteSelectionController;

    class NoteViewModelContextData : public QObject {
        Q_OBJECT
        Q_DECLARE_PUBLIC(ProjectViewModelContext)
    public:
        ProjectViewModelContext *q_ptr;

        Core::DspxDocument *document{};
        dspx::TrackList *trackList{};
        dspx::SelectionModel *selectionModel{};
        dspx::NoteSelectionModel *noteSelectionModel{};
        NoteSelectionController *noteSelectionController{};

        QHash<dspx::Track *, sflow::RangeSequenceViewModel *> singingClipPerTrackSequenceViewModelMap;
        QHash<sflow::RangeSequenceViewModel *, dspx::Track *> trackFromSingingClipSequenceViewModelMap;

        QHash<dspx::SingingClip *, sflow::RangeSequenceViewModel *> noteSequenceViewModelMap;
        QHash<sflow::RangeSequenceViewModel *, dspx::SingingClip *> singingClipFromNoteSequenceViewModelMap;

        QHash<dspx::Note *, sflow::NoteViewModel *> noteViewItemMap;
        QHash<sflow::NoteViewModel *, dspx::Note *> noteDocumentItemMap;
        QSet<sflow::NoteViewModel *> moveUpdatedNotes;
        QSet<sflow::NoteViewModel *> lengthUpdatedNotes;
        QSet<sflow::NoteViewModel *> lyricUpdatedNotes;
        QSet<sflow::NoteViewModel *> additionalTextUpdatedNotes;

        bool shouldCopyBeforeMove{};
        bool moveChanged{};
        bool lengthChanged{};
        bool lyricChanged{};
        bool additionalTextChanged{};
        bool drawChanged{};

        dspx::Note *targetNote{};
        dspx::SingingClip *targetSingingClip{};
        QQuickItem *targetNoteArea{};

        Core::TransactionController::TransactionId moveTransactionId{};
        Core::TransactionController::TransactionId adjustTransactionId{};
        Core::TransactionController::TransactionId drawTransactionId{};
        Core::TransactionController::TransactionId lyricTransactionId{};
        Core::TransactionController::TransactionId additionalTextTransactionId{};

        QStateMachine *stateMachine{};
        QState *idleState{};
        QState *rubberBandDraggingState{};
        QState *movePendingState{};
        QState *moveProcessingState{};
        QState *moveCommittingState{};
        QState *moveAbortingState{};
        QState *adjustPendingState{};
        QState *adjustProcessingState{};
        QState *adjustCommittingState{};
        QState *adjustAbortingState{};
        QState *drawPendingState{};
        QState *drawProcessingState{};
        QState *drawCommittingState{};
        QState *drawAbortingState{};
        QState *splitPendingState{};
        QState *splitCommittingState{};
        QState *lyricPendingState{};
        QState *lyricProgressingState{};
        QState *lyricCommittingState{};
        QState *lyricAbortingState{};
        QState *additionalTextPendingState{};
        QState *additionalTextProgressingState{};
        QState *additionalTextCommittingState{};
        QState *additionalTextAbortingState{};

        int splitPosition{};

        void init();
        void initStateMachine();
        void bindTrackSequences();
        void bindTrack(dspx::Track *track);
        void unbindTrack(dspx::Track *track);
        void bindSingingClip(dspx::SingingClip *clip);
        void unbindSingingClip(dspx::SingingClip *clip);
        void bindNoteDocumentItem(dspx::Note *item);
        void unbindNoteDocumentItem(dspx::Note *item);
        sflow::NoteEditLayerInteractionController *createController(QObject *parent);

        void onMovePendingStateEntered();
        void onMoveCommittingStateEntered();
        void onMoveAbortingStateEntered();

        void onAdjustPendingStateEntered();
        void onAdjustCommittingStateEntered();
        void onAdjustAbortingStateEntered();

        void onDrawPendingStateEntered();
        void onDrawCommittingStateEntered();
        void onDrawAbortingStateEntered();

        void onSplitCommittingStateEntered();

        void onLyricPendingStateEntered();
        void onLyricCommittingStateEntered();
        void onLyricAbortingStateEntered();

        void onAdditionalTextPendingStateEntered();
        void onAdditionalTextCommittingStateEntered();
        void onAdditionalTextAbortingStateEntered();

    Q_SIGNALS:
        void rubberBandDragWillStart();
        void rubberBandDragWillFinish();

        void moveTransactionWillStart();
        void moveTransactionStarted();
        void moveTransactionNotStarted();
        void moveTransactionWillCommit();
        void moveTransactionWillAbort();

        void adjustTransactionWillStart();
        void adjustTransactionStarted();
        void adjustTransactionNotStarted();
        void adjustTransactionWillCommit();
        void adjustTransactionWillAbort();

        void drawTransactionWillStart();
        void drawTransactionStarted();
        void drawTransactionNotStarted();
        void drawTransactionWillCommit();
        void drawTransactionWillAbort();

        void splitWillStart();
        void splitWillCommit();
        void splitWillAbort();

        void lyricTransactionWillStart();
        void lyricTransactionStarted();
        void lyricTransactionNotStarted();
        void lyricTransactionWillCommit();
        void lyricTransactionWillAbort();

        void additionalTextTransactionWillStart();
        void additionalTextTransactionStarted();
        void additionalTextTransactionNotStarted();
        void additionalTextTransactionWillCommit();
        void additionalTextTransactionWillAbort();
    };

}

#endif //DIFFSCOPE_VISUALEDITOR_NOTEVIEWMODELCONTEXTDATA_P_H
