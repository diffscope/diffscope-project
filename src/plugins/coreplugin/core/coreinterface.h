#ifndef DIFFSCOPE_COREPLUGIN_COREINTERFACE_H
#define DIFFSCOPE_COREPLUGIN_COREINTERFACE_H

#include <QObject>
#include <QSettings>
#include <qqmlintegration.h>

#include <CoreApi/coreinterfacebase.h>

#include <QAKCore/actionregistry.h>

#include <coreplugin/coreglobal.h>

class QQmlEngine;
class QJSEngine;

namespace Core {

    namespace Internal {
        class CorePlugin;
    }

    class ProjectWindowInterface;

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

        Q_INVOKABLE static int execSettingsDialog(const QString &id, QWindow *parent);
        Q_INVOKABLE static void execPluginsDialog(QWindow *parent);
        Q_INVOKABLE static void execAboutAppDialog(QWindow *parent);
        Q_INVOKABLE static void execAboutQtDialog(QWindow *parent);
        Q_INVOKABLE static void showHome();

    public:
        Q_INVOKABLE static ProjectWindowInterface *newFile();
        Q_INVOKABLE static bool openFile(const QString &fileName, QWidget *parent = nullptr);

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
