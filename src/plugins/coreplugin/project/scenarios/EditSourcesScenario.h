#ifndef DIFFSCOPE_COREPLUGIN_EDITSOURCESSCENARIO_H
#define DIFFSCOPE_COREPLUGIN_EDITSOURCESSCENARIO_H

#include <QList>
#include <QObject>
#include <QScopedPointer>
#include <qqmlintegration.h>

#include <dspxmodelORM/SingingClip.h>

#include <coreplugin/coreglobal.h>

class QQuickWindow;

namespace Core {

    class DspxDocument;
    class SourcesPickerModel;
    class EditSourcesScenarioPrivate;

    class CORE_EXPORT EditSourcesScenario : public QObject {
        Q_OBJECT
        QML_ELEMENT
        Q_DECLARE_PRIVATE(EditSourcesScenario)
        Q_PROPERTY(QQuickWindow *window READ window WRITE setWindow NOTIFY windowChanged)
        Q_PROPERTY(DspxDocument *document READ document WRITE setDocument NOTIFY documentChanged)

    public:
        explicit EditSourcesScenario(QObject *parent = nullptr);
        ~EditSourcesScenario() override;

        QQuickWindow *window() const;
        void setWindow(QQuickWindow *window);

        DspxDocument *document() const;
        void setDocument(DspxDocument *document);

        Q_INVOKABLE void editSources(SourcesPickerModel *initialModel, const QList<dspx::SingingClip *> &clips);
        Q_INVOKABLE void applySources(SourcesPickerModel *model, const QList<dspx::SingingClip *> &clips);

    Q_SIGNALS:
        void windowChanged();
        void documentChanged();

    private Q_SLOTS:
        void handleDialogAccepted();

    private:
        QScopedPointer<EditSourcesScenarioPrivate> d_ptr;
    };

}

#endif // DIFFSCOPE_COREPLUGIN_EDITSOURCESSCENARIO_H
