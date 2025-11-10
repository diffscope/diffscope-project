#include "ArrangementPanelInterface.h"
#include "ArrangementPanelInterface_p.h"

#include <coreplugin/ProjectWindowInterface.h>

namespace VisualEditor {

    ArrangementPanelInterface::ArrangementPanelInterface(Core::ProjectWindowInterface *windowHandle) : QObject(windowHandle), d_ptr(new ArrangementPanelInterfacePrivate) {
        Q_D(ArrangementPanelInterface);
        Q_ASSERT(windowHandle->getObjects(staticMetaObject.className()).isEmpty());
        windowHandle->addObject(staticMetaObject.className(), this);
        d->q_ptr = this;
        d->windowHandle = windowHandle;
    }

    ArrangementPanelInterface::~ArrangementPanelInterface() = default;

    ArrangementPanelInterface *ArrangementPanelInterface::of(const Core::ProjectWindowInterface *windowHandle) {
        return qobject_cast<ArrangementPanelInterface *>(windowHandle->getFirstObject(staticMetaObject.className()));
    }

}

#include "moc_ArrangementPanelInterface.cpp"