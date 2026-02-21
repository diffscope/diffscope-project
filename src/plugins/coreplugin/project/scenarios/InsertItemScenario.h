#ifndef DIFFSCOPE_COREPLUGIN_INSERTITEMSCENARIO_H
#define DIFFSCOPE_COREPLUGIN_INSERTITEMSCENARIO_H

#include <coreplugin/DocumentEditScenario.h>

namespace dspx {
    class Track;
    class Label;
}

namespace Core {

    class ProjectTimeline;

    class InsertItemScenarioPrivate;

    class CORE_EXPORT InsertItemScenario : public DocumentEditScenario {
        Q_OBJECT
        QML_ELEMENT
        Q_DECLARE_PRIVATE(InsertItemScenario)

        Q_PROPERTY(ProjectTimeline *projectTimeline READ projectTimeline WRITE setProjectTimeline NOTIFY projectTimelineChanged)

    public:
        explicit InsertItemScenario(QObject *parent = nullptr);
        ~InsertItemScenario() override;

        ProjectTimeline *projectTimeline() const;
        void setProjectTimeline(ProjectTimeline *projectTimeline);

        Q_INVOKABLE void addTrack() const;
        Q_INVOKABLE void insertTrack() const;
        Q_INVOKABLE void insertLabel() const;
        Q_INVOKABLE void insertSingingClip() const;
        Q_INVOKABLE void insertNote() const;

    Q_SIGNALS:
        void projectTimelineChanged();

    private:
        QScopedPointer<InsertItemScenarioPrivate> d_ptr;
    };

}

#endif //DIFFSCOPE_COREPLUGIN_INSERTITEMSCENARIO_H