#ifndef DIFFSCOPE_COREPLUGIN_EDITACTIONSHANDLERREGISTRY_P_H
#define DIFFSCOPE_COREPLUGIN_EDITACTIONSHANDLERREGISTRY_P_H

#include <QList>

#include <coreplugin/editactionshandlerregistry.h>

namespace Core {

    class EditActionsHandlerRegistryPrivate {
        Q_DECLARE_PUBLIC(EditActionsHandlerRegistry)
    public:
        EditActionsHandlerRegistry *q_ptr;
        QList<EditActionsHandler *> handlers;
        EditActionsHandler *currentHandler = nullptr;

        void updateCurrentHandler();
        void connectHandler(EditActionsHandler *handler);
        void disconnectHandler(EditActionsHandler *handler);
        void onHandlerDestroyed(QObject *handler);
    };

}

#endif //DIFFSCOPE_COREPLUGIN_EDITACTIONSHANDLERREGISTRY_P_H
