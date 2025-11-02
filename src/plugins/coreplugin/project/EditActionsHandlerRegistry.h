#ifndef DIFFSCOPE_COREPLUGIN_EDITACTIONSHANDLERREGISTRY_H
#define DIFFSCOPE_COREPLUGIN_EDITACTIONSHANDLERREGISTRY_H

#include <qqmlintegration.h>

#include <coreplugin/coreglobal.h>
#include <coreplugin/EditActionsHandler.h>

namespace Core {

    class EditActionsHandlerRegistryPrivate;

    class CORE_EXPORT EditActionsHandlerRegistry : public EditActionsHandler {
        Q_OBJECT
        QML_ELEMENT
        Q_DECLARE_PRIVATE(EditActionsHandlerRegistry)

    public:
        explicit EditActionsHandlerRegistry(QObject *parent = nullptr);
        ~EditActionsHandlerRegistry() override;

        Q_INVOKABLE bool pushHandler(EditActionsHandler *handler);
        Q_INVOKABLE bool popHandler(EditActionsHandler *handler);

        EditAction enabledActions() const override;
        bool triggerEditAction(EditAction action) override;
        MoveDirection enabledMoveDirections() const override;
        bool move(MoveDirection direction, int offset) override;

    private:
        QScopedPointer<EditActionsHandlerRegistryPrivate> d_ptr;
    };

}

#endif //DIFFSCOPE_COREPLUGIN_EDITACTIONSHANDLERREGISTRY_H
