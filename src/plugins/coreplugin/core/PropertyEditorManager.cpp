#include "PropertyEditorManager.h"
#include "PropertyEditorManager_p.h"

#include <QQmlComponent>

namespace Core {

    PropertyEditorManager::PropertyEditorManager(QObject *parent)
        : QObject(parent), d_ptr(new PropertyEditorManagerPrivate) {
        Q_D(PropertyEditorManager);
        d->q_ptr = this;
    }

    PropertyEditorManager::~PropertyEditorManager() = default;

    void PropertyEditorManager::addNoneComponent(QQmlComponent *component) {
        Q_D(PropertyEditorManager);
        d->noneComponents.append(component);
    }

    void PropertyEditorManager::addAnchorNodeComponent(QQmlComponent *component) {
        Q_D(PropertyEditorManager);
        d->anchorNodeComponents.append(component);
    }

    void PropertyEditorManager::addClipComponent(QQmlComponent *component) {
        Q_D(PropertyEditorManager);
        d->clipComponents.append(component);
    }

    void PropertyEditorManager::addLabelComponent(QQmlComponent *component) {
        Q_D(PropertyEditorManager);
        d->labelComponents.append(component);
    }

    void PropertyEditorManager::addNoteComponent(QQmlComponent *component) {
        Q_D(PropertyEditorManager);
        d->noteComponents.append(component);
    }

    void PropertyEditorManager::addTempoComponent(QQmlComponent *component) {
        Q_D(PropertyEditorManager);
        d->tempoComponents.append(component);
    }

    void PropertyEditorManager::addTrackComponent(QQmlComponent *component) {
        Q_D(PropertyEditorManager);
        d->trackComponents.append(component);
    }

    QList<QQmlComponent *> PropertyEditorManager::noneComponents() const {
        Q_D(const PropertyEditorManager);
        return d->noneComponents;
    }

    QList<QQmlComponent *> PropertyEditorManager::anchorNodeComponents() const {
        Q_D(const PropertyEditorManager);
        return d->anchorNodeComponents;
    }

    QList<QQmlComponent *> PropertyEditorManager::clipComponents() const {
        Q_D(const PropertyEditorManager);
        return d->clipComponents;
    }

    QList<QQmlComponent *> PropertyEditorManager::labelComponents() const {
        Q_D(const PropertyEditorManager);
        return d->labelComponents;
    }

    QList<QQmlComponent *> PropertyEditorManager::noteComponents() const {
        Q_D(const PropertyEditorManager);
        return d->noteComponents;
    }

    QList<QQmlComponent *> PropertyEditorManager::tempoComponents() const {
        Q_D(const PropertyEditorManager);
        return d->tempoComponents;
    }

    QList<QQmlComponent *> PropertyEditorManager::trackComponents() const {
        Q_D(const PropertyEditorManager);
        return d->trackComponents;
    }

}

#include "moc_PropertyEditorManager.cpp"
