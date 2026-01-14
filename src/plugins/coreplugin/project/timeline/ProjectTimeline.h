#ifndef DIFFSCOPE_COREPLUGIN_PROJECTTIMELINE_H
#define DIFFSCOPE_COREPLUGIN_PROJECTTIMELINE_H

#include <qqmlintegration.h>

#include <QObject>

#include <coreplugin/coreglobal.h>

// Forward declarations
namespace SVS {
    class MusicTimeline;
}

namespace Core {

    class DspxDocument;

    class ProjectTimelinePrivate;

    class CORE_EXPORT ProjectTimeline : public QObject {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_DECLARE_PRIVATE(ProjectTimeline)

        Q_PROPERTY(SVS::MusicTimeline *musicTimeline READ musicTimeline CONSTANT)
        Q_PROPERTY(int position READ position WRITE setPosition NOTIFY positionChanged)
        Q_PROPERTY(int lastPosition READ lastPosition WRITE setLastPosition NOTIFY lastPositionChanged)
        Q_PROPERTY(int rangeHint READ rangeHint WRITE setRangeHint NOTIFY rangeHintChanged)

    public:
        explicit ProjectTimeline(DspxDocument *document, QObject *parent = nullptr);
        ~ProjectTimeline() override;

        SVS::MusicTimeline *musicTimeline() const;

        int position() const;
        void setPosition(int position);

        int lastPosition() const;
        void setLastPosition(int lastPosition);

        int rangeHint() const;
        void setRangeHint(int rangeHint);

        Q_INVOKABLE void goTo(int position);

    Q_SIGNALS:
        void positionChanged(int position);
        void lastPositionChanged(int lastPosition);
        void rangeHintChanged(int rangeHint);

    private:
        QScopedPointer<ProjectTimelinePrivate> d_ptr;
    };

}

#endif //DIFFSCOPE_COREPLUGIN_PROJECTTIMELINE_H
