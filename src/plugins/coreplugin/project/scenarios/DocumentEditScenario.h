#ifndef DIFFSCOPE_COREPLUGIN_DOCUMENTEDITSCENARIO_H
#define DIFFSCOPE_COREPLUGIN_DOCUMENTEDITSCENARIO_H

#include <QObject>
#include <qqmlintegration.h>

#include <coreplugin/coreglobal.h>

class QQmlComponent;
class QQuickWindow;

namespace Core {

    class DspxDocument;

    class DocumentEditScenarioPrivate;

    class CORE_EXPORT DocumentEditScenario : public QObject {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_DECLARE_PRIVATE(DocumentEditScenario)

        Q_PROPERTY(QQuickWindow *window READ window WRITE setWindow NOTIFY windowChanged)
        Q_PROPERTY(DspxDocument *document READ document WRITE setDocument NOTIFY documentChanged)
        Q_PROPERTY(bool shouldDialogPopupAtCursor READ shouldDialogPopupAtCursor WRITE setShouldDialogPopupAtCursor NOTIFY shouldDialogPopupAtCursorChanged)

    public:
        ~DocumentEditScenario() override;

        QQuickWindow *window() const;
        void setWindow(QQuickWindow *window);

        DspxDocument *document() const;
        void setDocument(DspxDocument *document);

        bool shouldDialogPopupAtCursor() const;
        void setShouldDialogPopupAtCursor(bool shouldDialogPopupAtCursor);

    Q_SIGNALS:
        void windowChanged();
        void documentChanged();
        void shouldDialogPopupAtCursorChanged();

    protected:
        explicit DocumentEditScenario(QObject *parent = nullptr);
        DocumentEditScenario(DocumentEditScenarioPrivate &d, QObject *parent = nullptr);

        QObject *createAndPositionDialog(QQmlComponent *component, const QVariantMap &initialProperties) const;

    private:
        QScopedPointer<DocumentEditScenarioPrivate> d_ptr;
    };

}

#endif //DIFFSCOPE_COREPLUGIN_DOCUMENTEDITSCENARIO_H
