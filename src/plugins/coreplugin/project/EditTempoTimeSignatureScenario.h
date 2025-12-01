#ifndef DIFFSCOPE_COREPLUGIN_EDITTEMPOTIMESIGNATURESCENARIO_H
#define DIFFSCOPE_COREPLUGIN_EDITTEMPOTIMESIGNATURESCENARIO_H

#include <QObject>
#include <qqmlintegration.h>

#include <coreplugin/coreglobal.h>

class QQuickWindow;

namespace Core {

    class ProjectTimeline;
    class DspxDocument;

    class EditTempoTimeSignatureScenarioPrivate;

    class CORE_EXPORT EditTempoTimeSignatureScenario : public QObject {
        Q_OBJECT
        QML_ELEMENT
        Q_DECLARE_PRIVATE(EditTempoTimeSignatureScenario)

        Q_PROPERTY(QQuickWindow *window READ window WRITE setWindow NOTIFY windowChanged)
        Q_PROPERTY(ProjectTimeline *projectTimeline READ projectTimeline WRITE setProjectTimeline NOTIFY projectTimelineChanged)
        Q_PROPERTY(DspxDocument *document READ document WRITE setDocument NOTIFY documentChanged)

    public:
        explicit EditTempoTimeSignatureScenario(QObject *parent = nullptr);
        ~EditTempoTimeSignatureScenario() override;

        QQuickWindow *window() const;
        void setWindow(QQuickWindow *window);

        ProjectTimeline *projectTimeline() const;
        void setProjectTimeline(ProjectTimeline *projectTimeline);

        DspxDocument *document() const;
        void setDocument(DspxDocument *document);

        Q_INVOKABLE void editTempo() const;
        Q_INVOKABLE void editTempo(int position, bool doInsertNew, double initialTempo) const;
        Q_INVOKABLE void insertTempoAt(int position) const;
        Q_INVOKABLE void modifyExistingTempoAt(int position) const;

        Q_INVOKABLE void editTimeSignature() const;
        Q_INVOKABLE void editTimeSignature(int position, bool doInsertNew, int initialNumerator, int initialDenominator) const;
        Q_INVOKABLE void insertTimeSignatureAt(int position) const;
        Q_INVOKABLE void modifyExistingTimeSignatureAt(int position) const;


    Q_SIGNALS:
        void windowChanged();
        void projectTimelineChanged();
        void documentChanged();

    private:
        QScopedPointer<EditTempoTimeSignatureScenarioPrivate> d_ptr;
    };

}

#endif //DIFFSCOPE_COREPLUGIN_EDITTEMPOTIMESIGNATURESCENARIO_H
