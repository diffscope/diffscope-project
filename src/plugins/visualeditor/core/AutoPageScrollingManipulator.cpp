#include "AutoPageScrollingManipulator.h"
#include "AutoPageScrollingManipulator_p.h"

#include <QQuickItem>

#include <ScopicFlowCore/TimeViewModel.h>
#include <ScopicFlowCore/TimeLayoutViewModel.h>
#include <ScopicFlowCore/PlaybackViewModel.h>

namespace VisualEditor {

    AutoPageScrollingManipulator::AutoPageScrollingManipulator(QObject *parent)
        : QObject(parent), d_ptr(new AutoPageScrollingManipulatorPrivate) {
        Q_D(AutoPageScrollingManipulator);
        d->q_ptr = this;
        d->timeViewModel = nullptr;
        d->timeLayoutViewModel = nullptr;
        d->playbackViewModel = nullptr;
        d->target = nullptr;
        d->explicitViewSize = 0.0;
        d->isViewSizeExplicitlySet = false;
        d->enabled = false;
    }

    AutoPageScrollingManipulator::~AutoPageScrollingManipulator() = default;

    sflow::TimeViewModel *AutoPageScrollingManipulator::timeViewModel() const {
        Q_D(const AutoPageScrollingManipulator);
        return d->timeViewModel;
    }

    void AutoPageScrollingManipulator::setTimeViewModel(sflow::TimeViewModel *timeViewModel) {
        Q_D(AutoPageScrollingManipulator);
        if (d->timeViewModel == timeViewModel)
            return;

        d->timeViewModel = timeViewModel;
        Q_EMIT timeViewModelChanged();
    }

    sflow::TimeLayoutViewModel *AutoPageScrollingManipulator::timeLayoutViewModel() const {
        Q_D(const AutoPageScrollingManipulator);
        return d->timeLayoutViewModel;
    }

    void AutoPageScrollingManipulator::setTimeLayoutViewModel(sflow::TimeLayoutViewModel *timeLayoutViewModel) {
        Q_D(AutoPageScrollingManipulator);
        if (d->timeLayoutViewModel == timeLayoutViewModel)
            return;

        d->timeLayoutViewModel = timeLayoutViewModel;
        Q_EMIT timeLayoutViewModelChanged();
    }

    sflow::PlaybackViewModel *AutoPageScrollingManipulator::playbackViewModel() const {
        Q_D(const AutoPageScrollingManipulator);
        return d->playbackViewModel;
    }

    void AutoPageScrollingManipulator::setPlaybackViewModel(sflow::PlaybackViewModel *playbackViewModel) {
        Q_D(AutoPageScrollingManipulator);
        if (d->playbackViewModel == playbackViewModel)
            return;

        // Disconnect old connections
        d->disconnectPlaybackViewModel();

        d->playbackViewModel = playbackViewModel;

        // Connect new connections
        d->connectPlaybackViewModel();

        Q_EMIT playbackViewModelChanged();
    }

    QQuickItem *AutoPageScrollingManipulator::target() const {
        Q_D(const AutoPageScrollingManipulator);
        return d->target;
    }

    void AutoPageScrollingManipulator::setTarget(QQuickItem *target) {
        Q_D(AutoPageScrollingManipulator);
        if (d->target == target)
            return;

        // Disconnect old connections
        d->disconnectTarget();

        d->target = target;

        // Connect new connections
        d->connectTarget();

        Q_EMIT targetChanged();
        Q_EMIT viewSizeChanged();
    }

    double AutoPageScrollingManipulator::viewSize() const {
        Q_D(const AutoPageScrollingManipulator);
        return d->getViewSize();
    }

    void AutoPageScrollingManipulator::setViewSize(double viewSize) {
        Q_D(AutoPageScrollingManipulator);
        d->isViewSizeExplicitlySet = true;
        if (qFuzzyCompare(d->explicitViewSize, viewSize))
            return;

        d->explicitViewSize = viewSize;
        Q_EMIT viewSizeChanged();
    }

    void AutoPageScrollingManipulator::resetViewSize() {
        Q_D(AutoPageScrollingManipulator);
        d->isViewSizeExplicitlySet = false;
        Q_EMIT viewSizeChanged();
    }

    bool AutoPageScrollingManipulator::enabled() const {
        Q_D(const AutoPageScrollingManipulator);
        return d->enabled;
    }

    void AutoPageScrollingManipulator::setEnabled(bool enabled) {
        Q_D(AutoPageScrollingManipulator);
        if (d->enabled == enabled)
            return;

        d->enabled = enabled;
        Q_EMIT enabledChanged();
    }

    double AutoPageScrollingManipulatorPrivate::getViewSize() const {
        if (isViewSizeExplicitlySet) {
            return explicitViewSize;
        } else {
            return target ? target->width() : 0.0;
        }
    }

    void AutoPageScrollingManipulatorPrivate::updatePageTurning() {
        if (!enabled || getViewSize() <= 0 || !timeViewModel || !timeLayoutViewModel || !playbackViewModel)
            return;

        int primaryPosition = playbackViewModel->primaryPosition();
        double start = timeViewModel->start();
        double viewSizeInTicks = getViewSize() / timeLayoutViewModel->pixelDensity();

        if (primaryPosition < start) {
            // Move view to show the current position on the left side
            double newStart = qMax(0.0, primaryPosition - viewSizeInTicks);
            timeViewModel->setStart(newStart);
        } else if (primaryPosition > start + viewSizeInTicks) {
            // Move view to show the current position at the start
            timeViewModel->setStart(static_cast<double>(primaryPosition));
        }
    }

    void AutoPageScrollingManipulatorPrivate::connectPlaybackViewModel() {
        Q_Q(AutoPageScrollingManipulator);
        if (!playbackViewModel)
            return;
        QObject::connect(playbackViewModel, &sflow::PlaybackViewModel::primaryPositionChanged, q, [=, this] {
            updatePageTurning();
        });
    }

    void AutoPageScrollingManipulatorPrivate::disconnectPlaybackViewModel() {
        Q_Q(AutoPageScrollingManipulator);
        if (!playbackViewModel)
            return;
        QObject::disconnect(playbackViewModel, nullptr, q, nullptr);
    }

    void AutoPageScrollingManipulatorPrivate::connectTarget() {
        Q_Q(AutoPageScrollingManipulator);
        if (!target)
            return;
        QObject::connect(target, &QQuickItem::widthChanged, q, [=, this] {
            if (!isViewSizeExplicitlySet) {
                Q_EMIT q->viewSizeChanged();
            }
        });
    }

    void AutoPageScrollingManipulatorPrivate::disconnectTarget() {
        Q_Q(AutoPageScrollingManipulator);
        if (!target)
            return;
        QObject::disconnect(target, nullptr, q, nullptr);
    }

}

#include "moc_AutoPageScrollingManipulator.cpp"
#include "moc_AutoPageScrollingManipulator_p.cpp"
