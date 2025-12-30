#ifndef DIFFSCOPE_VISUALEDITOR_PLAYBACKVIEWMODELCONTEXTDATA_P_H
#define DIFFSCOPE_VISUALEDITOR_PLAYBACKVIEWMODELCONTEXTDATA_P_H

#include <QObject>

#include <visualeditor/ProjectViewModelContext.h>

class QState;
class QStateMachine;
class QQuickItem;

namespace sflow {
    class TimelineInteractionController;
}

namespace VisualEditor {

    class PlaybackViewModelContextData : public QObject {
        Q_OBJECT
        Q_DECLARE_PUBLIC(ProjectViewModelContext)
    public:
        ProjectViewModelContext *q_ptr;

        Core::ProjectWindowInterface *windowHandle;
        sflow::PlaybackViewModel *playbackViewModel;

        QStateMachine *stateMachine;
        QState *idleState;
        QState *rubberBandDraggingState;
        QState *positionIndicatorMovingState;

        void init();
        void initStateMachine();
        void bindPlaybackViewModel();
        sflow::TimelineInteractionController *createController(QObject *parent);

        void onPositionIndicatorMovingStateEntered();
        void onPositionIndicatorMovingStateExited();

    Q_SIGNALS:
        void rubberBandDragWillStart();
        void rubberBandDragWillFinish();

        void positionIndicatorMoveWillStart();
        void positionIndicatorMoveWillFinish();
    };

}

#endif //DIFFSCOPE_VISUALEDITOR_PLAYBACKVIEWMODELCONTEXTDATA_P_H
