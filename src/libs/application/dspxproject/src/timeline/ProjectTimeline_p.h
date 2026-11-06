// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_DSPXPROJECT_PROJECTTIMELINE_P_H
#define DIFFSCOPE_DSPXPROJECT_PROJECTTIMELINE_P_H

#include <dspxproject/ProjectTimeline.h>

#include <QHash>
#include <QSet>

namespace dspx {
    class Tempo;
    class TimeSignature;
}

namespace DspxProject {

    class ProjectTimelinePrivate {
        Q_DECLARE_PUBLIC(ProjectTimeline)
    public:
        ProjectTimeline *q_ptr;

        Document *document;

        SVS::MusicTimeline *musicTimeline{};
        int position{};
        int lastPosition{};
        int rangeHint{1};

        QHash<int, QSet<dspx::Tempo *>> tempoMap;
        QHash<dspx::Tempo *, int> tempoPosMap;
        QHash<int, QSet<dspx::TimeSignature *>> timeSignatureMap;
        QHash<dspx::TimeSignature *, int> timeSignatureMeasureMap;

        void handleTempoInsertedOrUpdated(dspx::Tempo *tempo);
        void handleTempoRemoved(dspx::Tempo *tempo);

        void handleTimeSignatureInsertedOrUpdated(dspx::TimeSignature *timeSignature);
        void handleTimeSignatureRemoved(dspx::TimeSignature *timeSignature);
    };

}

#endif // DIFFSCOPE_DSPXPROJECT_PROJECTTIMELINE_P_H
