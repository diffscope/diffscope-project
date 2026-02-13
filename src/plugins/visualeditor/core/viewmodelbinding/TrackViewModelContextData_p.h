#ifndef DIFFSCOPE_VISUALEDITOR_TRACKVIEWMODELCONTEXTDATA_P_H
#define DIFFSCOPE_VISUALEDITOR_TRACKVIEWMODELCONTEXTDATA_P_H

#include <QColor>
#include <QHash>
#include <QObject>

#include <transactional/TransactionController.h>
#include <visualeditor/ProjectViewModelContext.h>

class QState;
class QStateMachine;
class QQuickItem;

namespace dspx {
    class Track;
    class TrackList;
    class TrackSelectionModel;
}

namespace sflow {
    class ListViewModel;
    class TrackListInteractionController;
    class TrackViewModel;
}

namespace Core {
    class DspxDocument;
    class TrackColorSchema;
}

namespace VisualEditor {

    class TrackSelectionController;

    class TrackViewModelContextData : public QObject {
        Q_OBJECT
        Q_DECLARE_PUBLIC(ProjectViewModelContext)
    public:
        ProjectViewModelContext *q_ptr;

        Core::DspxDocument *document;
        dspx::TrackList *trackList;
        dspx::TrackSelectionModel *trackSelectionModel;

        Core::TrackColorSchema *trackColorSchema{};

        sflow::ListViewModel *trackListViewModel;
        TrackSelectionController *trackSelectionController;

        QHash<dspx::Track *, sflow::TrackViewModel *> trackViewItemMap;
        QHash<sflow::TrackViewModel *, dspx::Track *> trackDocumentItemMap;

        QStateMachine *stateMachine;
        QState *idleState;
        QState *rubberBandDraggingState;

        QState *movePendingState;
        QState *moveProcessingState;
        QState *moveCommittingState;
        QState *moveAbortingState;

        QState *mutePendingState;
        QState *muteEditingState;
        QState *muteFinishingState;

        QState *soloPendingState;
        QState *soloEditingState;
        QState *soloFinishingState;

        QState *recordPendingState;
        QState *recordEditingState;
        QState *recordFinishingState;

        QState *namePendingState;
        QState *nameProgressingState;
        QState *nameCommittingState;
        QState *nameAbortingState;

        QState *gainPendingState;
        QState *gainProgressingState;
        QState *gainCommittingState;
        QState *gainAbortingState;

        QState *panPendingState;
        QState *panProgressingState;
        QState *panCommittingState;
        QState *panAbortingState;

        QState *heightPendingState;
        QState *heightEditingState;
        QState *heightFinishingState;

        Core::TransactionController::TransactionId moveTransactionId{};
        Core::TransactionController::TransactionId muteTransactionId{};
        Core::TransactionController::TransactionId soloTransactionId{};
        Core::TransactionController::TransactionId recordTransactionId{};
        Core::TransactionController::TransactionId nameTransactionId{};
        Core::TransactionController::TransactionId gainTransactionId{};
        Core::TransactionController::TransactionId panTransactionId{};
        Core::TransactionController::TransactionId heightTransactionId{};

        dspx::Track *targetTrack{};
        bool moveChanged{false};

        void init();
        void initStateMachine();
        void bindTrackListViewModel();
        void bindTrackDocumentItem(int index, dspx::Track *item);
        void unbindTrackDocumentItem(int index, dspx::Track *item);
        sflow::TrackListInteractionController *createController(QObject *parent);

        void onItemColorIndicatorClicked(QQuickItem *trackListItem, int index);

        QColor trackColorForId(int colorId) const;
        void updateAllTrackColors();

        void selectAllClipsOnTrack(int index);

        void onMovePendingStateEntered();
        void onMoveCommittingStateEntered();
        void onMoveAbortingStateEntered();

        void onMutePendingStateEntered();
        void onMuteFinishingStateEntered();

        void onSoloPendingStateEntered();
        void onSoloFinishingStateEntered();

        void onRecordPendingStateEntered();
        void onRecordFinishingStateEntered();

        void onNamePendingStateEntered();
        void onNameCommittingStateEntered();
        void onNameAbortingStateEntered();

        void onGainPendingStateEntered();
        void onGainCommittingStateEntered();
        void onGainAbortingStateEntered();

        void onPanPendingStateEntered();
        void onPanCommittingStateEntered();
        void onPanAbortingStateEntered();

        void onHeightPendingStateEntered();
        void onHeightFinishingStateEntered();

    Q_SIGNALS:
        void rubberBandDragWillStart();
        void rubberBandDragWillFinish();

        void moveTransactionWillStart();
        void moveTransactionStarted();
        void moveTransactionNotStarted();
        void moveTransactionWillCommit();
        void moveTransactionWillAbort();

        void muteTransactionWillStart();
        void muteTransactionStarted();
        void muteTransactionNotStarted();
        void muteTransactionWillFinish();

        void soloTransactionWillStart();
        void soloTransactionStarted();
        void soloTransactionNotStarted();
        void soloTransactionWillFinish();

        void recordTransactionWillStart();
        void recordTransactionStarted();
        void recordTransactionNotStarted();
        void recordTransactionWillFinish();

        void nameTransactionWillStart();
        void nameTransactionStarted();
        void nameTransactionNotStarted();
        void nameTransactionWillCommit();
        void nameTransactionWillAbort();

        void gainTransactionWillStart();
        void gainTransactionStarted();
        void gainTransactionNotStarted();
        void gainTransactionWillCommit();
        void gainTransactionWillAbort();

        void panTransactionWillStart();
        void panTransactionStarted();
        void panTransactionNotStarted();
        void panTransactionWillCommit();
        void panTransactionWillAbort();

        void heightTransactionWillStart();
        void heightTransactionStarted();
        void heightTransactionNotStarted();
        void heightTransactionWillFinish();
    };

}

#endif //DIFFSCOPE_VISUALEDITOR_TRACKVIEWMODELCONTEXTDATA_P_H
