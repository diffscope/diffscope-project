// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "AutoPageScrollingManipulator.h"
#include "AutoPageScrollingManipulator_p.h"

#include <cmath>
#include <limits>

#include <QQuickItem>

#include <SVSCraftCore/MusicTime.h>
#include <SVSCraftCore/MusicTimeline.h>
#include <ScopicFlowCore/PlaybackViewModel.h>
#include <ScopicFlowCore/TimeLayoutViewModel.h>
#include <ScopicFlowCore/TimeViewModel.h>

#include <visualeditor/internal/EditorPreference.h>

namespace VisualEditor {

    static double tickToMilliseconds(const SVS::MusicTimeline *timeline, double tick) {
        const int wholeTick = static_cast<int>(qMin(tick, static_cast<double>(std::numeric_limits<int>::max())));
        const double wholeMilliseconds = timeline->create(0, 0, wholeTick).millisecond();
        const double millisecondsPerTick = 60000.0 / (timeline->ticksPerQuarterNote() * timeline->tempoAt(wholeTick));
        return wholeMilliseconds + (tick - wholeTick) * millisecondsPerTick;
    }

    AutoPageScrollingManipulator::AutoPageScrollingManipulator(QObject *parent)
        : QObject(parent), d_ptr(new AutoPageScrollingManipulatorPrivate) {
        Q_D(AutoPageScrollingManipulator);
        d->q_ptr = this;
        d->animation.setStartValue(0.0);
        d->animation.setEndValue(1.0);
        d->animation.setEasingCurve(QEasingCurve::InOutCubic);
        connect(&d->animation, &QVariantAnimation::valueChanged, this, [this, d](const QVariant &value) {
            if (d->animation.state() != QAbstractAnimation::Running || !d->timeViewModel || !d->playbackViewModel)
                return;
            const double progress = value.toDouble();
            const double newStart = qMax(0.0, d->playbackViewModel->primaryPosition() - d->animationRetainedLength);
            d->animatedStart = std::lerp(d->animationStart, newStart, progress);
            d->timeViewModel->setStart(d->animatedStart);
            if (d->animation.state() == QAbstractAnimation::Running)
                Q_EMIT pageTurnProgressed(progress);
        });
        connect(&d->animation, &QVariantAnimation::finished, this, [this, d] {
            if (d->timeViewModel && d->playbackViewModel)
                Q_EMIT pageTurned(d->timeViewModel->start(), d->animationViewLength);
        });
        connect(this, &AutoPageScrollingManipulator::viewSizeChanged, &d->animation, &QVariantAnimation::stop);
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

        d->animation.stop();
        if (d->timeViewModel)
            disconnect(d->timeViewModel, nullptr, this, nullptr);
        d->timeViewModel = timeViewModel;
        if (timeViewModel) {
            connect(timeViewModel, &sflow::TimeViewModel::startChanged, this, [d] {
                if (d->timeViewModel->start() != d->animatedStart)
                    d->animation.stop();
            });
        }
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

        d->animation.stop();
        if (d->timeLayoutViewModel)
            disconnect(d->timeLayoutViewModel, &sflow::TimeLayoutViewModel::pixelDensityChanged, &d->animation, &QVariantAnimation::stop);
        d->timeLayoutViewModel = timeLayoutViewModel;
        if (timeLayoutViewModel)
            connect(timeLayoutViewModel, &sflow::TimeLayoutViewModel::pixelDensityChanged, &d->animation, &QVariantAnimation::stop);
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

        d->animation.stop();
        d->disconnectPlaybackViewModel();

        d->playbackViewModel = playbackViewModel;

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
        const double oldViewSize = d->getViewSize();
        d->isViewSizeExplicitlySet = true;
        d->explicitViewSize = viewSize;
        if (oldViewSize != viewSize)
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
        if (!enabled)
            d->animation.stop();
        Q_EMIT enabledChanged();
    }

    bool AutoPageScrollingManipulator::isPlaying() const {
        Q_D(const AutoPageScrollingManipulator);
        return d->playing;
    }

    void AutoPageScrollingManipulator::setPlaying(bool playing) {
        Q_D(AutoPageScrollingManipulator);
        if (d->playing == playing)
            return;
        d->playing = playing;
        Q_EMIT playingChanged();
    }

    double AutoPageScrollingManipulatorPrivate::getViewSize() const {
        if (isViewSizeExplicitlySet) {
            return explicitViewSize;
        } else {
            return target ? target->width() : 0.0;
        }
    }

    void AutoPageScrollingManipulatorPrivate::updatePageTurning() {
        if (!enabled || !timeViewModel || !timeLayoutViewModel || !playbackViewModel)
            return;

        const double pixelDensity = timeLayoutViewModel->pixelDensity();
        if (pixelDensity <= 0.0)
            return;
        const double viewLength = getViewSize() / pixelDensity;
        if (viewLength <= 0.0 || !std::isfinite(viewLength))
            return;

        const bool useTriggerArea = playing || Internal::EditorPreference::autoPageScrollTriggerOutsidePlayback();
        const int triggerArea = useTriggerArea ? Internal::EditorPreference::autoPageScrollTriggerArea() : 0;
        const double triggerLength = viewLength * (triggerArea / 100.0);
        const double primaryPosition = playbackViewModel->primaryPosition();
        const auto shouldTurnPage = [=](double start) {
            const double end = start + viewLength;
            return primaryPosition < start || primaryPosition > end ||
                   (triggerLength > 0.0 && primaryPosition >= end - triggerLength);
        };
        if (animation.state() == QAbstractAnimation::Running) {
            if (shouldTurnPage(animationStart) && qAbs(primaryPosition - animationPosition) < viewLength)
                return;
            animation.stop();
        }

        const double start = timeViewModel->start();
        if (!shouldTurnPage(start))
            return;

        const double retainedLength = viewLength * (Internal::EditorPreference::autoPageScrollRetainedArea() / 100.0);
        const double newStart = qMax(0.0, primaryPosition - retainedLength);
        if (newStart == start || (primaryPosition >= start && newStart < start))
            return;

        const int duration = Internal::EditorPreference::autoPageScrollAnimationDuration();
        const auto timeline = timeViewModel->timeline();
        Q_Q(AutoPageScrollingManipulator);
        if (duration > 0 && timeline && tickToMilliseconds(timeline, newStart + viewLength) - tickToMilliseconds(timeline, newStart) >= 2.0 * duration) {
            animationStart = start;
            animationPosition = primaryPosition;
            animationViewLength = viewLength;
            animationRetainedLength = retainedLength;
            animatedStart = start;
            animation.setDuration(duration);
            double expectedStart = newStart;
            if (playing) {
                const double expectedTime = tickToMilliseconds(timeline, qMax(0.0, primaryPosition)) + duration;
                constexpr int maximumPosition = std::numeric_limits<int>::max();
                const int expectedPosition = expectedTime < tickToMilliseconds(timeline, maximumPosition) ? timeline->create(expectedTime).totalTick() : maximumPosition;
                expectedStart = qMax(0.0, expectedPosition - retainedLength);
            }
            Q_EMIT q->pageTurnStarted(expectedStart, viewLength);
            animation.start();
            return;
        }

        Q_EMIT q->pageTurnStarted(newStart, viewLength);
        timeViewModel->setStart(newStart);
        Q_EMIT q->pageTurnProgressed(1.0);
        Q_EMIT q->pageTurned(newStart, viewLength);
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
