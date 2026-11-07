// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_DSPXPROJECT_PROJECTTIMELINE_H
#define DIFFSCOPE_DSPXPROJECT_PROJECTTIMELINE_H

#include <qqmlintegration.h>

#include <QObject>

#include <dspxproject/dspxprojectglobal.h>

namespace SVS {
    class MusicTimeline;
}

namespace DspxProject {

    class Document;

    class ProjectTimelinePrivate;

    /** Mirrors project tempo and meter data and stores playhead positions in project ticks. */
    class DSPXPROJECT_EXPORT ProjectTimeline : public QObject {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_DECLARE_PRIVATE(ProjectTimeline)

        Q_PROPERTY(SVS::MusicTimeline *musicTimeline READ musicTimeline CONSTANT)
        Q_PROPERTY(int position READ position WRITE setPosition NOTIFY positionChanged)
        Q_PROPERTY(int lastPosition READ lastPosition WRITE setLastPosition NOTIFY lastPositionChanged)
        Q_PROPERTY(int rangeHint READ rangeHint WRITE setRangeHint NOTIFY rangeHintChanged)

    public:
        /**
         * Creates a timeline bound to document without taking ownership.
         * @pre document is non-null and outlives this timeline.
         */
        explicit ProjectTimeline(Document *document, QObject *parent = nullptr);
        /** Destroys the timeline and its music time mapping. */
        ~ProjectTimeline() override;

        /** Returns the non-null owned tempo and meter mapping. */
        SVS::MusicTimeline *musicTimeline() const;

        /** Returns the current playhead position in project ticks. */
        int position() const;
        /** Sets the current position and expands rangeHint() if needed. */
        void setPosition(int position);

        /** Returns the stored navigation or playback start position in project ticks. */
        int lastPosition() const;
        /** Sets the stored position and expands rangeHint() if needed. */
        void setLastPosition(int lastPosition);

        /** Returns the suggested timeline extent in project ticks. */
        int rangeHint() const;
        /** Sets the suggested timeline extent in project ticks. */
        void setRangeHint(int rangeHint);

        /** Sets both the current and stored positions to position in project ticks. */
        Q_INVOKABLE void goTo(int position);

    Q_SIGNALS:
        /** Notifies that the current playhead position changed. */
        void positionChanged(int position);
        /** Notifies that the stored position changed. */
        void lastPositionChanged(int lastPosition);
        /** Notifies that the suggested timeline extent changed. */
        void rangeHintChanged(int rangeHint);

    private:
        QScopedPointer<ProjectTimelinePrivate> d_ptr;
    };

}

#endif // DIFFSCOPE_DSPXPROJECT_PROJECTTIMELINE_H
