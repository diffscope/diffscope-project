#ifndef DIFFSCOPE_COREPLUGIN_INSERTITEMSCENARIO_H
#define DIFFSCOPE_COREPLUGIN_INSERTITEMSCENARIO_H

#include <QObject>
#include <qqmlintegration.h>

#include <coreplugin/coreglobal.h>

class QQuickWindow;

namespace dspx {
    class Track;
    class Label;
}

namespace Core {

    class ProjectTimeline;
    class DspxDocument;

    class InsertItemScenarioPrivate;

    class CORE_EXPORT InsertItemScenario : public QObject {
        Q_OBJECT
        QML_ELEMENT
        Q_DECLARE_PRIVATE(InsertItemScenario)

        Q_PROPERTY(QQuickWindow *window READ window WRITE setWindow NOTIFY windowChanged)
        Q_PROPERTY(ProjectTimeline *projectTimeline READ projectTimeline WRITE setProjectTimeline NOTIFY projectTimelineChanged)
        Q_PROPERTY(DspxDocument *document READ document WRITE setDocument NOTIFY documentChanged)
        Q_PROPERTY(bool shouldDialogPopupAtCursor READ shouldDialogPopupAtCursor WRITE setShouldDialogPopupAtCursor NOTIFY shouldDialogPopupAtCursorChanged)

    public:
        explicit InsertItemScenario(QObject *parent = nullptr);
        ~InsertItemScenario() override;

        QQuickWindow *window() const;
        void setWindow(QQuickWindow *window);

        ProjectTimeline *projectTimeline() const;
        void setProjectTimeline(ProjectTimeline *projectTimeline);

        DspxDocument *document() const;
        void setDocument(DspxDocument *document);

        bool shouldDialogPopupAtCursor() const;
        void setShouldDialogPopupAtCursor(bool shouldDialogPopupAtCursor);

        Q_INVOKABLE void addTrack() const;
        Q_INVOKABLE void insertTrack() const;
        Q_INVOKABLE void insertLabel() const;

    Q_SIGNALS:
        void windowChanged();
        void projectTimelineChanged();
        void documentChanged();
        void shouldDialogPopupAtCursorChanged();

    private:
        QScopedPointer<InsertItemScenarioPrivate> d_ptr;
    };

}

#endif //DIFFSCOPE_COREPLUGIN_INSERTITEMSCENARIO_H