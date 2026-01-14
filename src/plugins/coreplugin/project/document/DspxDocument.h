#ifndef DIFFSCOPE_COREPLUGIN_DSPXDOCUMENT_H
#define DIFFSCOPE_COREPLUGIN_DSPXDOCUMENT_H

#include <QObject>
#include <qqmlintegration.h>

#include <coreplugin/coreglobal.h>

namespace dspx {
    class Model;
    class SelectionModel;
}

namespace Core {

    class TransactionController;

    struct DspxClipboardData;

    class DspxDocumentPrivate;

    class CORE_EXPORT DspxDocument : public QObject {
        Q_OBJECT
        QML_ELEMENT
        Q_DECLARE_PRIVATE(DspxDocument)
        Q_PROPERTY(dspx::Model *model READ model CONSTANT)
        Q_PROPERTY(dspx::SelectionModel *selectionModel READ selectionModel CONSTANT)
        Q_PROPERTY(TransactionController *transactionController READ transactionController CONSTANT)
        Q_PROPERTY(bool anyItemsSelected READ anyItemsSelected NOTIFY anyItemsSelectedChanged)
        Q_PROPERTY(bool editScopeFocused READ isEditScopeFocused NOTIFY editScopeFocusedChanged)
        Q_PROPERTY(bool pasteAvailable READ pasteAvailable NOTIFY pasteAvailableChanged)
    public:
        explicit DspxDocument(QObject *parent = nullptr);
        ~DspxDocument() override;

        dspx::Model *model() const;
        dspx::SelectionModel *selectionModel() const;
        TransactionController *transactionController() const;

        bool anyItemsSelected();
        bool isEditScopeFocused();
        bool pasteAvailable();

        Q_INVOKABLE void cutSelection(int playheadPosition = 0);
        Q_INVOKABLE void copySelection(int playheadPosition = 0);
        Q_INVOKABLE void paste(int playheadPosition = 0);
        Q_INVOKABLE void deleteSelection();

    Q_SIGNALS:
        void anyItemsSelectedChanged();
        void editScopeFocusedChanged();
        void pasteAvailableChanged();

    private:
        QScopedPointer<DspxDocumentPrivate> d_ptr;
    };

}

#endif //DIFFSCOPE_COREPLUGIN_DSPXDOCUMENT_H
