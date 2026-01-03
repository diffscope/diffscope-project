#ifndef DIFFSCOPE_VISUALEDITOR_PLAYBACKVIEWMODELCONTEXTDATA_P_H
#define DIFFSCOPE_VISUALEDITOR_PLAYBACKVIEWMODELCONTEXTDATA_P_H

#include <QObject>

#include <transactional/TransactionController.h>

#include <visualeditor/ProjectViewModelContext.h>

class QState;
class QStateMachine;
class QQuickItem;

namespace sflow {
    class TimelineInteractionController;
}

namespace dspx {
    class Timeline;
}

namespace Core {
    class DspxDocument;
}

namespace VisualEditor {

    class PlaybackViewModelContextData : public QObject {
        Q_OBJECT
        Q_DECLARE_PUBLIC(ProjectViewModelContext)
    public:
        ProjectViewModelContext *q_ptr;

        Core::DspxDocument *document;
        Core::ProjectWindowInterface *windowHandle;
        dspx::Timeline *timeline;
        sflow::PlaybackViewModel *playbackViewModel;

        QStateMachine *stateMachine;
        QState *idleState;
        QState *rubberBandDraggingState;
        QState *positionIndicatorMovingState;
        QState *loopRangeAdjustPendingState;
        QState *loopRangeAdjustingState;

        Core::TransactionController::TransactionId loopRangeTransactionId{};
        int pendingLoopStart{};
        int pendingLoopLength{};

        void init();
        void initStateMachine();
        void bindPlaybackViewModel();
        sflow::TimelineInteractionController *createController(QObject *parent);

        void onLoopRangeAdjustPendingStateEntered();
        void onLoopRangeAdjustingStateExited();
        void onPositionIndicatorMovingStateEntered();
        void onPositionIndicatorMovingStateExited();

    Q_SIGNALS:
        void rubberBandDragWillStart();
        void rubberBandDragWillFinish();

        void positionIndicatorMoveWillStart();
        void positionIndicatorMoveWillFinish();

        void loopRangeTransactionWillStart();
        void loopRangeTransactionStarted();
        void loopRangeTransactionNotStarted();
        void loopRangeTransactionWillFinish();
    };

}

#endif //DIFFSCOPE_VISUALEDITOR_PLAYBACKVIEWMODELCONTEXTDATA_P_H
