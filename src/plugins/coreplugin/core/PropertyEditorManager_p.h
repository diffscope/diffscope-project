#ifndef DIFFSCOPE_COREPLUGIN_PROPERTYEDITORMANAGER_P_H
#define DIFFSCOPE_COREPLUGIN_PROPERTYEDITORMANAGER_P_H

#include "PropertyEditorManager.h"

#include <QList>

class QQmlComponent;

namespace Core {

    class PropertyEditorManagerPrivate {
        Q_DECLARE_PUBLIC(PropertyEditorManager)
    public:
        PropertyEditorManager *q_ptr;

        QList<QQmlComponent *> noneComponents;
        QList<QQmlComponent *> anchorNodeComponents;
        QList<QQmlComponent *> clipComponents;
        QList<QQmlComponent *> keySignatureComponents;
        QList<QQmlComponent *> labelComponents;
        QList<QQmlComponent *> noteComponents;
        QList<QQmlComponent *> tempoComponents;
        QList<QQmlComponent *> trackComponents;
    };

}

#endif // DIFFSCOPE_COREPLUGIN_PROPERTYEDITORMANAGER_P_H
