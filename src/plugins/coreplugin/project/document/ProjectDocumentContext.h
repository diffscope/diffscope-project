#ifndef DIFFSCOPE_COREPLUGIN_PROJECTDOCUMENTCONTEXT_H
#define DIFFSCOPE_COREPLUGIN_PROJECTDOCUMENTCONTEXT_H

#include <QObject>

#include <coreplugin/coreglobal.h>

class QWindow;

namespace QDspx {
    struct Model;
}

namespace Core {

    class DspxDocument;
    class FileLocker;
    class OpenSaveProjectFileScenario;

    class ProjectDocumentContextPrivate;

    class CORE_EXPORT ProjectDocumentContext : public QObject {
        Q_OBJECT
        Q_DECLARE_PRIVATE(ProjectDocumentContext)
        Q_PROPERTY(FileLocker *fileLocker READ fileLocker CONSTANT)
        Q_PROPERTY(DspxDocument *document READ document CONSTANT)
        Q_PROPERTY(OpenSaveProjectFileScenario *openSaveProjectFileScenario READ openSaveProjectFileScenario CONSTANT)
        Q_PROPERTY(QString defaultDocumentName READ defaultDocumentName NOTIFY defaultDocumentNameChanged)
    public:
        explicit ProjectDocumentContext(QObject *parent = nullptr);
        ~ProjectDocumentContext() override;

        FileLocker *fileLocker() const;

        DspxDocument *document() const;

        OpenSaveProjectFileScenario *openSaveProjectFileScenario() const;

        QString defaultDocumentName() const;

        bool openFile(const QString &filePath);
        bool newFile(const QDspx::Model &templateModel, const QString &defaultDocumentName, bool isNonFileDocument);
        bool newFile(const QString &templateFilePath, const QString &defaultDocumentName, bool isNonFileDocument);
        bool save();
        bool saveAs(const QString &filePath);
        bool saveCopy(const QString &filePath);

    Q_SIGNALS:
        void saved();
        void defaultDocumentNameChanged();

    private:
        QScopedPointer<ProjectDocumentContextPrivate> d_ptr;
    };

}

#endif //DIFFSCOPE_COREPLUGIN_PROJECTDOCUMENTCONTEXT_H
