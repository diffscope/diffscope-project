#ifndef DIFFSCOPE_VISUALEDITOR_MASTERTRACKVIEWMODELCONTEXTDATA_P_H
#define DIFFSCOPE_VISUALEDITOR_MASTERTRACKVIEWMODELCONTEXTDATA_P_H

#include <QObject>

#include <transactional/TransactionController.h>
#include <visualeditor/ProjectViewModelContext.h>

class QState;
class QStateMachine;
class QQuickItem;

namespace dspx {
    class Master;
}

namespace sflow {
    class ListViewModel;
    class TrackListInteractionController;
    class TrackViewModel;
}

namespace Core {
    class DspxDocument;
}

namespace VisualEditor {

    class MasterTrackViewModelContextData : public QObject {
        Q_OBJECT
        Q_DECLARE_PUBLIC(ProjectViewModelContext)
    public:
        ProjectViewModelContext *q_ptr{};

        Core::DspxDocument *document{};
        dspx::Master *master{};

        sflow::ListViewModel *masterTrackListViewModel{};
        sflow::TrackViewModel *masterTrackViewModel{};

        QStateMachine *stateMachine{};
        QState *idleState{};

        QState *mutePendingState{};
        QState *muteEditingState{};
        QState *muteFinishingState{};

        QState *gainPendingState{};
        QState *gainProgressingState{};
        QState *gainCommittingState{};
        QState *gainAbortingState{};

        QState *panPendingState{};
        QState *panProgressingState{};
        QState *panCommittingState{};
        QState *panAbortingState{};

        QState *multiChannelPendingState{};
        QState *multiChannelEditingState{};
        QState *multiChannelFinishingState{};

        Core::TransactionController::TransactionId muteTransactionId{};
        Core::TransactionController::TransactionId gainTransactionId{};
        Core::TransactionController::TransactionId panTransactionId{};
        Core::TransactionController::TransactionId multiChannelTransactionId{};

        bool syncingFromModel{false};

        void init();
        void initStateMachine();
        void bindMasterTrackViewModel();
        sflow::TrackListInteractionController *createController(QObject *parent);

        void onMutePendingStateEntered();
        void onMuteFinishingStateEntered();

        void onGainPendingStateEntered();
        void onGainCommittingStateEntered();
        void onGainAbortingStateEntered();

        void onPanPendingStateEntered();
        void onPanCommittingStateEntered();
        void onPanAbortingStateEntered();

        void onMultiChannelPendingStateEntered();
        void onMultiChannelFinishingStateEntered();

    Q_SIGNALS:
        void muteTransactionWillStart();
        void muteTransactionStarted();
        void muteTransactionNotStarted();
        void muteTransactionWillFinish();

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

        void multiChannelTransactionWillStart();
        void multiChannelTransactionStarted();
        void multiChannelTransactionNotStarted();
        void multiChannelTransactionWillFinish();
    };

}

#endif //DIFFSCOPE_VISUALEDITOR_MASTERTRACKVIEWMODELCONTEXTDATA_P_H
