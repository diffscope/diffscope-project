#ifndef DIFFSCOPE_COREPLUGIN_OPENSAVEPROJECTFILESCENARIO_H
#define DIFFSCOPE_COREPLUGIN_OPENSAVEPROJECTFILESCENARIO_H

#include <QObject>
#include <qqmlintegration.h>

class QWindow;

namespace Core {

    class OpenSaveProjectFileScenarioPrivate;

    class OpenSaveProjectFileScenario : public QObject {
        Q_OBJECT
        QML_ELEMENT
        Q_DECLARE_PRIVATE(OpenSaveProjectFileScenario)
        Q_PROPERTY(QWindow *window READ window WRITE setWindow NOTIFY windowChanged)
    public:
        explicit OpenSaveProjectFileScenario(QObject *parent = nullptr);
        ~OpenSaveProjectFileScenario() override;

        QWindow *window() const;
        void setWindow(QWindow *window);

        Q_INVOKABLE static QString dspxFileFilter(bool withAllFiles = false);
        Q_INVOKABLE QString openProjectFile(const QString &defaultDir = {}) const;
        Q_INVOKABLE QString saveProjectFile(const QString &defaultDir = {}) const;
        Q_INVOKABLE void showOpenFailMessageBox(const QString &path, const QString &error) const;
        Q_INVOKABLE void showSaveFailMessageBox(const QString &path, const QString &error) const;
        Q_INVOKABLE void showDeserializationFailMessageBox(const QString &path) const;

    Q_SIGNALS:
        void windowChanged();

    private:
        QScopedPointer<OpenSaveProjectFileScenarioPrivate> d_ptr;
    };

}

#endif //DIFFSCOPE_COREPLUGIN_OPENSAVEPROJECTFILESCENARIO_H
