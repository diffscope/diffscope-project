#ifndef DIFFSCOPE_COREPLUGIN_EDITMETADATASCENARIO_H
#define DIFFSCOPE_COREPLUGIN_EDITMETADATASCENARIO_H

#include <QObject>
#include <qqmlintegration.h>

#include <coreplugin/coreglobal.h>

class QQuickWindow;

namespace Core {

    class DspxDocument;

    class EditMetadataScenarioPrivate;

    class CORE_EXPORT EditMetadataScenario : public QObject {
        Q_OBJECT
        QML_ELEMENT
        Q_DECLARE_PRIVATE(EditMetadataScenario)

        Q_PROPERTY(QQuickWindow *window READ window WRITE setWindow NOTIFY windowChanged)
        Q_PROPERTY(DspxDocument *document READ document WRITE setDocument NOTIFY documentChanged)
        Q_PROPERTY(bool shouldDialogPopupAtCursor READ shouldDialogPopupAtCursor WRITE setShouldDialogPopupAtCursor NOTIFY shouldDialogPopupAtCursorChanged)

    public:
        explicit EditMetadataScenario(QObject *parent = nullptr);
        ~EditMetadataScenario() override;

        QQuickWindow *window() const;
        void setWindow(QQuickWindow *window);

        DspxDocument *document() const;
        void setDocument(DspxDocument *document);

        bool shouldDialogPopupAtCursor() const;
        void setShouldDialogPopupAtCursor(bool shouldDialogPopupAtCursor);

        Q_INVOKABLE void editMetadata() const;

    Q_SIGNALS:
        void windowChanged();
        void documentChanged();
        void shouldDialogPopupAtCursorChanged();

    private:
        QScopedPointer<EditMetadataScenarioPrivate> d_ptr;
    };

}

#endif //DIFFSCOPE_COREPLUGIN_EDITMETADATASCENARIO_H
