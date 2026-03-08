#ifndef DIFFSCOPE_VISUALEDITOR_AUTOPAGESCROLLINGMANIPULATOR_P_H
#define DIFFSCOPE_VISUALEDITOR_AUTOPAGESCROLLINGMANIPULATOR_P_H

#include <QObject>
#include <visualeditor/AutoPageScrollingManipulator.h>

class QQuickItem;

namespace sflow {
    class TimeViewModel;
    class TimeLayoutViewModel;
    class PlaybackViewModel;
}

namespace VisualEditor {

    class AutoPageScrollingManipulatorPrivate;

    class AutoPageScrollingManipulatorPrivate {
        Q_DECLARE_PUBLIC(AutoPageScrollingManipulator)
    public:
        AutoPageScrollingManipulator *q_ptr;

        sflow::TimeViewModel *timeViewModel;
        sflow::TimeLayoutViewModel *timeLayoutViewModel;
        sflow::PlaybackViewModel *playbackViewModel;
        QQuickItem *target;
        double explicitViewSize;
        bool isViewSizeExplicitlySet;
        bool enabled;

        double getViewSize() const;
        void updatePageTurning();
        void connectPlaybackViewModel();
        void disconnectPlaybackViewModel();
        void connectTarget();
        void disconnectTarget();
    };

}

#endif //DIFFSCOPE_VISUALEDITOR_AUTOPAGESCROLLINGMANIPULATOR_P_H