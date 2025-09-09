#include "editactionshandlerregistry.h"
#include "editactionshandlerregistry_p.h"

namespace Core {

    EditActionsHandlerRegistry::EditActionsHandlerRegistry(QObject *parent) 
        : EditActionsHandler(parent), d_ptr(new EditActionsHandlerRegistryPrivate) {
        Q_D(EditActionsHandlerRegistry);
        d->q_ptr = this;
    }

    EditActionsHandlerRegistry::~EditActionsHandlerRegistry() = default;

    bool EditActionsHandlerRegistry::pushHandler(EditActionsHandler *handler) {
        Q_D(EditActionsHandlerRegistry);
        
        if (!handler) {
            return false;
        }

        // Remove if already exists to avoid duplicates
        for (auto it = d->handlers.begin(); it != d->handlers.end(); ++it) {
            if (*it == handler) {
                d->handlers.erase(it);
                break;
            }
        }

        // Add to end of list
        d->handlers.append(handler);
        d->connectHandler(handler);
        d->updateCurrentHandler();
        
        return true;
    }

    bool EditActionsHandlerRegistry::popHandler(EditActionsHandler *handler) {
        Q_D(EditActionsHandlerRegistry);
        
        if (!handler) {
            return false;
        }

        bool found = false;
        for (auto it = d->handlers.begin(); it != d->handlers.end(); ++it) {
            if (*it == handler) {
                d->disconnectHandler(handler);
                d->handlers.erase(it);
                found = true;
                break;
            }
        }

        if (found) {
            d->updateCurrentHandler();
        }

        return found;
    }

    EditActionsHandler::EditAction EditActionsHandlerRegistry::enabledActions() const {
        Q_D(const EditActionsHandlerRegistry);
        
        if (d->currentHandler) {
            return d->currentHandler->enabledActions();
        }
        
        return EditAction();
    }

    bool EditActionsHandlerRegistry::triggerEditAction(EditAction action) {
        Q_D(EditActionsHandlerRegistry);
        
        if (d->currentHandler) {
            return d->currentHandler->triggerEditAction(action);
        }
        
        return false;
    }

    EditActionsHandler::MoveDirection EditActionsHandlerRegistry::enabledMoveDirections() const {
        Q_D(const EditActionsHandlerRegistry);
        
        if (d->currentHandler) {
            return d->currentHandler->enabledMoveDirections();
        }
        
        return MoveDirection();
    }

    bool EditActionsHandlerRegistry::move(MoveDirection direction, int offset) {
        Q_D(EditActionsHandlerRegistry);
        
        if (d->currentHandler) {
            return d->currentHandler->move(direction, offset);
        }
        
        return false;
    }

    void EditActionsHandlerRegistryPrivate::updateCurrentHandler() {
        Q_Q(EditActionsHandlerRegistry);
        
        EditActionsHandler *newCurrentHandler = nullptr;
        
        // Find the last handler in the list
        if (!handlers.isEmpty()) {
            newCurrentHandler = handlers.last();
        }
        
        if (currentHandler != newCurrentHandler) {
            currentHandler = newCurrentHandler;
            
            // Emit signals for property changes
            emit q->enabledActionsChanged(q->enabledActions());
            emit q->enabledMoveDirectionsChanged(q->enabledMoveDirections());
        }
    }

    void EditActionsHandlerRegistryPrivate::connectHandler(EditActionsHandler *handler) {
        Q_Q(EditActionsHandlerRegistry);
        
        if (!handler) {
            return;
        }
        
        // Connect signals to update when the handler's properties change
        QObject::connect(handler, &EditActionsHandler::enabledActionsChanged,
                        q, [this, q]() {
                            if (currentHandler == qobject_cast<EditActionsHandler*>(q->sender())) {
                                emit q->enabledActionsChanged(q->enabledActions());
                            }
                        });
        
        QObject::connect(handler, &EditActionsHandler::enabledMoveDirectionsChanged,
                        q, [this, q]() {
                            if (currentHandler == qobject_cast<EditActionsHandler*>(q->sender())) {
                                emit q->enabledMoveDirectionsChanged(q->enabledMoveDirections());
                            }
                        });
        
        // Handle handler destruction
        QObject::connect(handler, &QObject::destroyed, q, [this](QObject *obj) {
            onHandlerDestroyed(obj);
        });
    }

    void EditActionsHandlerRegistryPrivate::disconnectHandler(EditActionsHandler *handler) {
        if (handler) {
            QObject::disconnect(handler, nullptr, q_ptr, nullptr);
        }
    }

    void EditActionsHandlerRegistryPrivate::onHandlerDestroyed(QObject *handler) {
        // Find and remove the destroyed handler from the list
        auto destroyedHandler = static_cast<EditActionsHandler *>(handler);
        handlers.removeAll(destroyedHandler);
        
        // Update current handler if needed
        if (currentHandler == destroyedHandler) {
            updateCurrentHandler();
        }
    }

}
