#ifndef DIFFSCOPE_COREPLUGIN_EDITLOOPSCENARIO_H
#define DIFFSCOPE_COREPLUGIN_EDITLOOPSCENARIO_H

#include <coreplugin/DocumentEditScenario.h>

namespace Core {

    class ProjectTimeline;

    class EditLoopScenarioPrivate;

    class CORE_EXPORT EditLoopScenario : public DocumentEditScenario {
        Q_OBJECT
        QML_ELEMENT
        Q_DECLARE_PRIVATE(EditLoopScenario)

        Q_PROPERTY(ProjectTimeline *projectTimeline READ projectTimeline WRITE setProjectTimeline NOTIFY projectTimelineChanged)

    public:
        explicit EditLoopScenario(QObject *parent = nullptr);
        ~EditLoopScenario() override;

        ProjectTimeline *projectTimeline() const;
        void setProjectTimeline(ProjectTimeline *projectTimeline);

        Q_INVOKABLE void editLoop() const;

    Q_SIGNALS:
        void projectTimelineChanged();

    private:
        QScopedPointer<EditLoopScenarioPrivate> d_ptr;
    };

}

#endif //DIFFSCOPE_COREPLUGIN_EDITLOOPSCENARIO_H
