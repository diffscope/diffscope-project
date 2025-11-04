#ifndef DIFFSCOPE_COREPLUGIN_PROJECTDOCUMENTCONTEXT_H
#define DIFFSCOPE_COREPLUGIN_PROJECTDOCUMENTCONTEXT_H

#include <QObject>

class QWindow;

namespace QDspx {
    struct Model;
}

namespace Core {

    class DspxDocument;
    class FileLocker;
    class OpenSaveProjectFileScenario;

    class ProjectDocumentContextPrivate;

    class ProjectDocumentContext : public QObject {
        Q_OBJECT
        Q_DECLARE_PRIVATE(ProjectDocumentContext)
        Q_PROPERTY(FileLocker *fileLocker READ fileLocker CONSTANT)
        Q_PROPERTY(DspxDocument *document READ document CONSTANT)
        Q_PROPERTY(OpenSaveProjectFileScenario *openSaveProjectFileScenario READ openSaveProjectFileScenario CONSTANT)
    public:
        explicit ProjectDocumentContext(QObject *parent = nullptr);
        ~ProjectDocumentContext() override;

        FileLocker *fileLocker() const;

        DspxDocument *document() const;

        OpenSaveProjectFileScenario *openSaveProjectFileScenario() const;

        bool openFile(const QString &filePath);
        void newFile(const QDspx::Model &templateModel, bool isNonFileDocument);
        bool newFile(const QString &templateFilePath, bool isNonFileDocument);
        bool save();
        bool saveAs(const QString &filePath);
        bool saveCopy(const QString &filePath);

    private:
        QScopedPointer<ProjectDocumentContextPrivate> d_ptr;
    };

}

#endif //DIFFSCOPE_COREPLUGIN_PROJECTDOCUMENTCONTEXT_H
