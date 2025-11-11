#include "PositionAlignmentManipulator.h"
#include "PositionAlignmentManipulator_p.h"

#include <ScopicFlowCore/TimeLayoutViewModel.h>

namespace VisualEditor {

    // Static helper function to find the appropriate note duration for auto mode
    static PositionAlignmentManipulator::Duration findNoteDurationInRange(double minValue, double maxValue) {
        // Define all note durations in descending order (longest to shortest)
        static const QList<PositionAlignmentManipulator::Duration> noteDurations = {
            PositionAlignmentManipulator::Note1st,
            PositionAlignmentManipulator::Note2nd, 
            PositionAlignmentManipulator::Note4th,
            PositionAlignmentManipulator::Note8th,
            PositionAlignmentManipulator::Note16th,
            PositionAlignmentManipulator::Note32nd,
            PositionAlignmentManipulator::Note64th,
            PositionAlignmentManipulator::Note128th
        };

        // Find the first note duration that falls within the range [minValue, maxValue)
        for (auto noteDuration : noteDurations) {
            double noteValue = static_cast<double>(noteDuration);
            if (noteValue >= minValue && noteValue < maxValue) {
                return noteDuration;
            }
        }

        // If no note duration is in range, return the boundary values
        if (maxValue <= static_cast<double>(PositionAlignmentManipulator::Note128th)) {
            return PositionAlignmentManipulator::Note128th; // shortest
        } else {
            return PositionAlignmentManipulator::Note1st; // longest
        }
    }

    PositionAlignmentManipulator::PositionAlignmentManipulator(QObject *parent)
        : QObject(parent), d_ptr(new PositionAlignmentManipulatorPrivate) {
        Q_D(PositionAlignmentManipulator);
        d->q_ptr = this;
        d->timeLayoutViewModel = nullptr;
        d->duration = Note8th;
        d->tuplet = None;
        d->autoDurationPositionAlignment = 48;
        d->helper = new PositionAlignmentManipulatorHelper(d, this);
    }

    PositionAlignmentManipulator::~PositionAlignmentManipulator() = default;

    sflow::TimeLayoutViewModel *PositionAlignmentManipulator::timeLayoutViewModel() const {
        Q_D(const PositionAlignmentManipulator);
        return d->timeLayoutViewModel;
    }

    void PositionAlignmentManipulator::setTimeLayoutViewModel(sflow::TimeLayoutViewModel *timeLayoutViewModel) {
        Q_D(PositionAlignmentManipulator);
        if (d->timeLayoutViewModel == timeLayoutViewModel)
            return;

        // Disconnect old connections
        d->disconnectTimeLayoutViewModel();

        d->timeLayoutViewModel = timeLayoutViewModel;

        // Connect new connections
        d->connectTimeLayoutViewModel();

        // Update position alignment with new view model
        d->updatePositionAlignment();

        Q_EMIT timeLayoutViewModelChanged();
    }

    PositionAlignmentManipulator::Duration PositionAlignmentManipulator::duration() const {
        Q_D(const PositionAlignmentManipulator);
        return d->duration;
    }

    void PositionAlignmentManipulator::setDuration(Duration duration) {
        Q_D(PositionAlignmentManipulator);
        if (d->duration == duration)
            return;

        d->duration = duration;
        d->updatePositionAlignment();
        Q_EMIT durationChanged();
    }

    PositionAlignmentManipulator::Tuplet PositionAlignmentManipulator::tuplet() const {
        Q_D(const PositionAlignmentManipulator);
        return d->tuplet;
    }

    void PositionAlignmentManipulator::setTuplet(Tuplet tuplet) {
        Q_D(PositionAlignmentManipulator);
        if (d->tuplet == tuplet)
            return;

        d->tuplet = tuplet;
        d->updatePositionAlignment();
        Q_EMIT tupletChanged();
    }

    int PositionAlignmentManipulator::autoDurationPositionAlignment() const {
        Q_D(const PositionAlignmentManipulator);
        return d->autoDurationPositionAlignment;
    }

    void PositionAlignmentManipulator::setAutoDurationPositionAlignment(int autoDurationPositionAlignment) {
        Q_D(PositionAlignmentManipulator);
        if (d->autoDurationPositionAlignment == autoDurationPositionAlignment)
            return;

        d->autoDurationPositionAlignment = autoDurationPositionAlignment;
        d->updatePositionAlignment();
        Q_EMIT autoDurationPositionAlignmentChanged();
    }

    void PositionAlignmentManipulatorPrivate::updatePositionAlignment() {
        Q_Q(PositionAlignmentManipulator);
        
        if (!timeLayoutViewModel)
            return;

        int newAlignment = 1; // Default value

        if (duration == PositionAlignmentManipulator::Unset) {
            newAlignment = 1;
        } else if (duration != PositionAlignmentManipulator::Auto) {
            // Calculate: duration * 2 / tuplet
            newAlignment = static_cast<int>(duration) * 2 / static_cast<int>(tuplet);
        } else {
            // Auto mode: find appropriate note duration based on pixel density
            double pixelDensity = timeLayoutViewModel->pixelDensity();
            if (pixelDensity > 0) {
                double minValue = autoDurationPositionAlignment / pixelDensity;
                double maxValue = 2.0 * autoDurationPositionAlignment / pixelDensity;
                
                auto foundDuration = findNoteDurationInRange(minValue, maxValue);
                newAlignment = static_cast<int>(foundDuration) * 2 / static_cast<int>(tuplet);
            }
        }

        timeLayoutViewModel->setPositionAlignment(newAlignment);
    }

    void PositionAlignmentManipulatorPrivate::connectTimeLayoutViewModel() const {
        if (!timeLayoutViewModel)
            return;

        // Connect to pixelDensity changes to recalculate auto mode alignment
        QObject::connect(timeLayoutViewModel, SIGNAL(pixelDensityChanged()), helper, SLOT(onPixelDensityChanged()));
    }

    void PositionAlignmentManipulatorPrivate::disconnectTimeLayoutViewModel() const {
        if (!timeLayoutViewModel)
            return;

        // Disconnect all connections from the old timeLayoutViewModel
        QObject::disconnect(timeLayoutViewModel, nullptr, helper, nullptr);
    }

    PositionAlignmentManipulatorHelper::PositionAlignmentManipulatorHelper(PositionAlignmentManipulatorPrivate *d, QObject *parent)
        : QObject(parent), d(d) {
    }

    void PositionAlignmentManipulatorHelper::onPixelDensityChanged() const {
        if (d->duration == PositionAlignmentManipulator::Auto) {
            d->updatePositionAlignment();
        }
    }

}

#include "moc_PositionAlignmentManipulator.cpp"
#include "moc_PositionAlignmentManipulator_p.cpp"
