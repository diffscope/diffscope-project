#ifndef DIFFSCOPE_COREPLUGIN_COREINTERFACE_H
#define DIFFSCOPE_COREPLUGIN_COREINTERFACE_H

#include <QObject>
#include <qqmlintegration.h>

#include <CoreApi/coreinterfacebase.h>

#include <coreplugin/coreglobal.h>

class QQmlEngine;
class QJSEngine;
class QWindow;

namespace QAK {
    class ActionRegistry;
}

namespace Core {

    namespace Internal {
        class CorePlugin;
    }

    class ProjectWindowInterface;
    class DspxCheckerRegistry;
    class ProjectDocumentContext;

    class CoreInterfacePrivate;

    class CORE_EXPORT CoreInterface : public CoreInterfaceBase {
        Q_OBJECT
        QML_ELEMENT
        QML_SINGLETON
        Q_DECLARE_PRIVATE(CoreInterface)
        Q_PROPERTY(QAK::ActionRegistry *actionRegistry READ actionRegistry CONSTANT)
    public:
        static CoreInterface *instance();
        static inline CoreInterface *create(QQmlEngine *, QJSEngine *) { return instance(); }

        static QAK::ActionRegistry *actionRegistry();

        static DspxCheckerRegistry *dspxCheckerRegistry();

        static constexpr const char *dspxEditorId() {
            return "org.diffscope.diffscope";
        }

        Q_INVOKABLE static int execSettingsDialog(const QString &id, QWindow *parent);
        Q_INVOKABLE static void execPluginsDialog(QWindow *parent);
        Q_INVOKABLE static void execAboutAppDialog(QWindow *parent);
        Q_INVOKABLE static void execAboutQtDialog(QWindow *parent);
        Q_INVOKABLE static void showHome();

    public:
        Q_INVOKABLE static ProjectWindowInterface *newFile(QWindow *parent = nullptr);
        Q_INVOKABLE static ProjectWindowInterface *newFileFromTemplate(const QString &templateFilePath, QWindow *parent = nullptr);
        Q_INVOKABLE static ProjectWindowInterface *openFile(const QString &filePath, QWindow *parent = nullptr);
        Q_INVOKABLE static ProjectWindowInterface *createProjectWindow(ProjectDocumentContext *projectDocumentContext);

    Q_SIGNALS:
        void resetAllDoNotShowAgainRequested();

    private:
        explicit CoreInterface(QObject *parent = nullptr);
        ~CoreInterface();

        CoreInterface(CoreInterfacePrivate &d, QObject *parent = nullptr);

        friend class Internal::CorePlugin;
    };

}

#endif // DIFFSCOPE_COREPLUGIN_COREINTERFACE_H
