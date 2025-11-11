#ifndef DIFFSCOPE_VISUALEDITOR_POSITIONALIGNMENTMANIPULATOR_P_H
#define DIFFSCOPE_VISUALEDITOR_POSITIONALIGNMENTMANIPULATOR_P_H

#include <QObject>
#include <visualeditor/PositionAlignmentManipulator.h>

namespace sflow {
    class TimeLayoutViewModel;
}

namespace VisualEditor {

    class PositionAlignmentManipulatorPrivate;

    class PositionAlignmentManipulatorHelper : public QObject {
        Q_OBJECT
    public:
        explicit PositionAlignmentManipulatorHelper(PositionAlignmentManipulatorPrivate *d, QObject *parent = nullptr);

    public Q_SLOTS:
        void onPixelDensityChanged() const;

    private:
        PositionAlignmentManipulatorPrivate *d;
    };

    class PositionAlignmentManipulatorPrivate {
        Q_DECLARE_PUBLIC(PositionAlignmentManipulator)
    public:
        PositionAlignmentManipulator *q_ptr;

        sflow::TimeLayoutViewModel *timeLayoutViewModel;
        PositionAlignmentManipulator::Duration duration;
        PositionAlignmentManipulator::Tuplet tuplet;
        int autoDurationPositionAlignment;

        PositionAlignmentManipulatorHelper *helper;

        void updatePositionAlignment();
        void connectTimeLayoutViewModel() const;
        void disconnectTimeLayoutViewModel() const;
    };

}

#endif //DIFFSCOPE_VISUALEDITOR_POSITIONALIGNMENTMANIPULATOR_P_H