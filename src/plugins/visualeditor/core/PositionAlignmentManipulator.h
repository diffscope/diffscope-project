#ifndef DIFFSCOPE_VISUALEDITOR_POSITIONALIGNMENTMANIPULATOR_H
#define DIFFSCOPE_VISUALEDITOR_POSITIONALIGNMENTMANIPULATOR_H

#include <QObject>
#include <qqmlintegration.h>

#include <visualeditor/visualeditorglobal.h>

namespace sflow {
    class TimeLayoutViewModel;
}

namespace VisualEditor {

    class PositionAlignmentManipulatorPrivate;

    class VISUAL_EDITOR_EXPORT PositionAlignmentManipulator : public QObject {
        Q_OBJECT
        QML_ELEMENT
        Q_DECLARE_PRIVATE(PositionAlignmentManipulator)
        Q_PROPERTY(sflow::TimeLayoutViewModel *timeLayoutViewModel READ timeLayoutViewModel WRITE setTimeLayoutViewModel NOTIFY timeLayoutViewModelChanged)
        Q_PROPERTY(Duration duration READ duration WRITE setDuration NOTIFY durationChanged)
        Q_PROPERTY(Tuplet tuplet READ tuplet WRITE setTuplet NOTIFY tupletChanged)
        Q_PROPERTY(int autoDurationPositionAlignment READ autoDurationPositionAlignment WRITE setAutoDurationPositionAlignment NOTIFY autoDurationPositionAlignmentChanged)

    public:
        explicit PositionAlignmentManipulator(QObject *parent = nullptr);
        ~PositionAlignmentManipulator() override;

        sflow::TimeLayoutViewModel *timeLayoutViewModel() const;
        void setTimeLayoutViewModel(sflow::TimeLayoutViewModel *timeLayoutViewModel);

        enum Duration {
            Auto = 0,
            Unset = 1,
            Note1st = 1920,
            Note2nd = 960,
            Note4th = 480,
            Note8th = 240,
            Note16th = 120,
            Note32nd = 60,
            Note64th = 30,
            Note128th = 15
        };
        Q_ENUM(Duration)
        Duration duration() const;
        void setDuration(Duration duration);

        enum Tuplet {
            None = 2,
            Triplet = 3,
            Quintuplet = 5
        };
        Q_ENUM(Tuplet)
        Tuplet tuplet() const;
        void setTuplet(Tuplet tuplet);

        int autoDurationPositionAlignment() const;
        void setAutoDurationPositionAlignment(int autoDurationPositionAlignment);

    Q_SIGNALS:
        void timeLayoutViewModelChanged();
        void durationChanged();
        void tupletChanged();
        void autoDurationPositionAlignmentChanged();

    private:
        QScopedPointer<PositionAlignmentManipulatorPrivate> d_ptr;
    };

}

#endif //DIFFSCOPE_VISUALEDITOR_POSITIONALIGNMENTMANIPULATOR_H