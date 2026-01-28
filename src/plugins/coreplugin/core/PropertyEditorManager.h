#ifndef DIFFSCOPE_COREPLUGIN_PROPERTYEDITORMANAGER_H
#define DIFFSCOPE_COREPLUGIN_PROPERTYEDITORMANAGER_H

#include <QObject>
#include <qqmlintegration.h>

class QQmlComponent;

namespace Core {

    class PropertyEditorManagerPrivate;

    class PropertyEditorManager : public QObject {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_DECLARE_PRIVATE(PropertyEditorManager)

        Q_PROPERTY(QList<QQmlComponent *> noneComponents READ noneComponents CONSTANT)
        Q_PROPERTY(QList<QQmlComponent *> anchorNodeComponents READ anchorNodeComponents CONSTANT)
        Q_PROPERTY(QList<QQmlComponent *> clipComponents READ clipComponents CONSTANT)
        Q_PROPERTY(QList<QQmlComponent *> labelComponents READ labelComponents CONSTANT)
        Q_PROPERTY(QList<QQmlComponent *> noteComponents READ noteComponents CONSTANT)
        Q_PROPERTY(QList<QQmlComponent *> tempoComponents READ tempoComponents CONSTANT)
        Q_PROPERTY(QList<QQmlComponent *> trackComponents READ trackComponents CONSTANT)

    public:
        explicit PropertyEditorManager(QObject *parent = nullptr);
        ~PropertyEditorManager() override;

        void addNoneComponent(QQmlComponent *component);
        void addAnchorNodeComponent(QQmlComponent *component);
        void addClipComponent(QQmlComponent *component);
        void addLabelComponent(QQmlComponent *component);
        void addNoteComponent(QQmlComponent *component);
        void addTempoComponent(QQmlComponent *component);
        void addTrackComponent(QQmlComponent *component);

        QList<QQmlComponent *> noneComponents() const;
        QList<QQmlComponent *> anchorNodeComponents() const;
        QList<QQmlComponent *> clipComponents() const;
        QList<QQmlComponent *> labelComponents() const;
        QList<QQmlComponent *> noteComponents() const;
        QList<QQmlComponent *> tempoComponents() const;
        QList<QQmlComponent *> trackComponents() const;

    private:
        QScopedPointer<PropertyEditorManagerPrivate> d_ptr;
    };

}

#endif // DIFFSCOPE_COREPLUGIN_PROPERTYEDITORMANAGER_H
