// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_VISUALEDITOR_AUTOPAGESCROLLINGMANIPULATOR_P_H
#define DIFFSCOPE_VISUALEDITOR_AUTOPAGESCROLLINGMANIPULATOR_P_H

#include <visualeditor/AutoPageScrollingManipulator.h>

#include <QPointer>
#include <QVariantAnimation>

class QQuickItem;

namespace sflow {
    class TimeViewModel;
    class TimeLayoutViewModel;
    class PlaybackViewModel;
}

namespace VisualEditor {

    class AutoPageScrollingManipulatorPrivate {
        Q_DECLARE_PUBLIC(AutoPageScrollingManipulator)
    public:
        AutoPageScrollingManipulator *q_ptr{};

        QPointer<sflow::TimeViewModel> timeViewModel;
        QPointer<sflow::TimeLayoutViewModel> timeLayoutViewModel;
        QPointer<sflow::PlaybackViewModel> playbackViewModel;
        QPointer<QQuickItem> target;
        double explicitViewSize{};
        bool isViewSizeExplicitlySet{};
        bool enabled{};
        bool playing{};

        QVariantAnimation animation;
        double animationStart{};
        double animationPosition{};
        double animationViewLength{};
        double animationRetainedLength{};
        double animatedStart{};

        double getViewSize() const;
        void updatePageTurning();
        void connectPlaybackViewModel();
        void disconnectPlaybackViewModel();
        void connectTarget();
        void disconnectTarget();
    };

}

#endif //DIFFSCOPE_VISUALEDITOR_AUTOPAGESCROLLINGMANIPULATOR_P_H
