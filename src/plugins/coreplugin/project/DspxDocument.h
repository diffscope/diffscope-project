#ifndef DIFFSCOPE_COREPLUGIN_DSPXDOCUMENT_H
#define DIFFSCOPE_COREPLUGIN_DSPXDOCUMENT_H

#include <QObject>
#include <qqmlintegration.h>

#include <coreplugin/coreglobal.h>

namespace dspx {
    class Model;
}

namespace Core {

    class TransactionController;

    class DspxDocumentPrivate;

    class CORE_EXPORT DspxDocument : public QObject {
        Q_OBJECT
        QML_ELEMENT
        Q_DECLARE_PRIVATE(DspxDocument)
        Q_PROPERTY(dspx::Model *model READ model CONSTANT)
        Q_PROPERTY(TransactionController *transactionController READ transactionController CONSTANT)
    public:
        explicit DspxDocument(QObject *parent = nullptr);
        ~DspxDocument() override;

        dspx::Model *model() const;
        TransactionController *transactionController() const;

    private:
        QScopedPointer<DspxDocumentPrivate> d_ptr;
    };

}

#endif //DIFFSCOPE_COREPLUGIN_DSPXDOCUMENT_H
