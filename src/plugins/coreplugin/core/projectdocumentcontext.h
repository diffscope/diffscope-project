#ifndef DIFFSCOPE_COREPLUGIN_PROJECTDOCUMENTCONTEXT_H
#define DIFFSCOPE_COREPLUGIN_PROJECTDOCUMENTCONTEXT_H

#include <QObject>

class QWindow;

namespace QDspx {
    class Model;
}

namespace Core {

    class DspxDocument;
    class FileLocker;

    class ProjectDocumentContextPrivate;

    class ProjectDocumentContext : public QObject {
        Q_OBJECT
        Q_DECLARE_PRIVATE(ProjectDocumentContext)
        Q_PROPERTY(FileLocker *fileLocker READ fileLocker CONSTANT)
    public:
        explicit ProjectDocumentContext(QObject *parent = nullptr);
        ~ProjectDocumentContext() override;

        FileLocker *fileLocker() const;

        DspxDocument *document() const;

        bool openFile(const QString &filePath, QWindow *parent = nullptr);
        void newFile(const QDspx::Model &templateModel, bool isNonFileDocument, QWindow *parent = nullptr);
        bool newFile(const QString &templateFilePath, bool isNonFileDocument, QWindow *parent = nullptr);

    private:
        QScopedPointer<ProjectDocumentContextPrivate> d_ptr;

    };

}

#endif //DIFFSCOPE_COREPLUGIN_PROJECTDOCUMENTCONTEXT_H
