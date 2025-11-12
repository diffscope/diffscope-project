#ifndef DIFFSCOPE_VISUALEDITOR_POSITIONALIGNMENTMANIPULATOR_P_H
#define DIFFSCOPE_VISUALEDITOR_POSITIONALIGNMENTMANIPULATOR_P_H

#include <QObject>
#include <visualeditor/PositionAlignmentManipulator.h>

namespace sflow {
    class TimeLayoutViewModel;
}

namespace VisualEditor {

    class PositionAlignmentManipulatorPrivate;

    class PositionAlignmentManipulatorPrivate {
        Q_DECLARE_PUBLIC(PositionAlignmentManipulator)
    public:
        PositionAlignmentManipulator *q_ptr;

        sflow::TimeLayoutViewModel *timeLayoutViewModel;
        PositionAlignmentManipulator::Duration duration;
        PositionAlignmentManipulator::Tuplet tuplet;
        int autoDurationPositionAlignment;

        void updatePositionAlignment();
        void connectTimeLayoutViewModel();
        void disconnectTimeLayoutViewModel();
    };

}

#endif //DIFFSCOPE_VISUALEDITOR_POSITIONALIGNMENTMANIPULATOR_P_H