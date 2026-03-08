#ifndef DIFFSCOPE_COREPLUGIN_EDITTEMPOTIMESIGNATURESCENARIO_H
#define DIFFSCOPE_COREPLUGIN_EDITTEMPOTIMESIGNATURESCENARIO_H

#include <coreplugin/DocumentEditScenario.h>

namespace Core {

    class ProjectTimeline;

    class EditTempoTimeSignatureScenarioPrivate;

    class CORE_EXPORT EditTempoTimeSignatureScenario : public DocumentEditScenario {
        Q_OBJECT
        QML_ELEMENT
        Q_DECLARE_PRIVATE(EditTempoTimeSignatureScenario)

        Q_PROPERTY(ProjectTimeline *projectTimeline READ projectTimeline WRITE setProjectTimeline NOTIFY projectTimelineChanged)

    public:
        explicit EditTempoTimeSignatureScenario(QObject *parent = nullptr);
        ~EditTempoTimeSignatureScenario() override;

        ProjectTimeline *projectTimeline() const;
        void setProjectTimeline(ProjectTimeline *projectTimeline);

        Q_INVOKABLE void editTempo() const;
        Q_INVOKABLE void editTempo(int position, bool doInsertNew, double initialTempo) const;
        Q_INVOKABLE void insertTempoAt(int position) const;
        Q_INVOKABLE void modifyExistingTempoAt(int position) const;

        Q_INVOKABLE void editTimeSignature() const;
        Q_INVOKABLE void editTimeSignature(int position, bool doInsertNew, int initialNumerator, int initialDenominator) const;
        Q_INVOKABLE void insertTimeSignatureAt(int position) const;
        Q_INVOKABLE void modifyExistingTimeSignatureAt(int position) const;

    Q_SIGNALS:
        void projectTimelineChanged();

    private:
        QScopedPointer<EditTempoTimeSignatureScenarioPrivate> d_ptr;
    };

}

#endif //DIFFSCOPE_COREPLUGIN_EDITTEMPOTIMESIGNATURESCENARIO_H
