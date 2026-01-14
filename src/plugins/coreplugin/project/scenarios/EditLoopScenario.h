#ifndef DIFFSCOPE_COREPLUGIN_EDITLOOPSCENARIO_H
#define DIFFSCOPE_COREPLUGIN_EDITLOOPSCENARIO_H

#include <QObject>
#include <qqmlintegration.h>

#include <coreplugin/coreglobal.h>

class QQuickWindow;

namespace Core {

    class ProjectTimeline;
    class DspxDocument;

    class EditLoopScenarioPrivate;

    class CORE_EXPORT EditLoopScenario : public QObject {
        Q_OBJECT
        QML_ELEMENT
        Q_DECLARE_PRIVATE(EditLoopScenario)

        Q_PROPERTY(QQuickWindow *window READ window WRITE setWindow NOTIFY windowChanged)
        Q_PROPERTY(ProjectTimeline *projectTimeline READ projectTimeline WRITE setProjectTimeline NOTIFY projectTimelineChanged)
        Q_PROPERTY(DspxDocument *document READ document WRITE setDocument NOTIFY documentChanged)
        Q_PROPERTY(bool shouldDialogPopupAtCursor READ shouldDialogPopupAtCursor WRITE setShouldDialogPopupAtCursor NOTIFY shouldDialogPopupAtCursorChanged)

    public:
        explicit EditLoopScenario(QObject *parent = nullptr);
        ~EditLoopScenario() override;

        QQuickWindow *window() const;
        void setWindow(QQuickWindow *window);

        ProjectTimeline *projectTimeline() const;
        void setProjectTimeline(ProjectTimeline *projectTimeline);

        DspxDocument *document() const;
        void setDocument(DspxDocument *document);

        bool shouldDialogPopupAtCursor() const;
        void setShouldDialogPopupAtCursor(bool shouldDialogPopupAtCursor);

        Q_INVOKABLE void editLoop() const;

    Q_SIGNALS:
        void windowChanged();
        void projectTimelineChanged();
        void documentChanged();
        void shouldDialogPopupAtCursorChanged();

    private:
        QScopedPointer<EditLoopScenarioPrivate> d_ptr;
    };

}

#endif //DIFFSCOPE_COREPLUGIN_EDITLOOPSCENARIO_H
