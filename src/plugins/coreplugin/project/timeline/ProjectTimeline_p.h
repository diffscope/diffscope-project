#ifndef DIFFSCOPE_COREPLUGIN_PROJECTTIMELINE_P_H
#define DIFFSCOPE_COREPLUGIN_PROJECTTIMELINE_P_H

#include <coreplugin/ProjectTimeline.h>

#include <QHash>
#include <QSet>

namespace dspx {
    class Tempo;
    class TimeSignature;
}

namespace Core {

    class ProjectTimelinePrivate {
        Q_DECLARE_PUBLIC(ProjectTimeline)
    public:
        ProjectTimeline *q_ptr;

        DspxDocument *document;

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

#endif //DIFFSCOPE_COREPLUGIN_PROJECTTIMELINE_P_H
