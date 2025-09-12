#ifndef DIFFSCOPE_COREPLUGIN_ICORE_H
#define DIFFSCOPE_COREPLUGIN_ICORE_H

#include <QObject>
#include <QSettings>
#include <qqmlintegration.h>

#include <CoreApi/icorebase.h>

#include <QAKCore/actionregistry.h>

#include <coreplugin/coreglobal.h>

class QQmlEngine;
class QJSEngine;
class QQuickWindow;

namespace Core {

    namespace Internal {
        class CorePlugin;
    }

    class ICorePrivate;

    class CORE_EXPORT ICore : public ICoreBase {
        Q_OBJECT
        QML_ELEMENT
        QML_SINGLETON
        Q_DECLARE_PRIVATE(ICore)
        Q_PROPERTY(QAK::ActionRegistry *actionRegistry READ actionRegistry CONSTANT)
    public:
        static ICore *instance();
        static inline ICore *create(QQmlEngine *, QJSEngine *) { return instance(); }

        static QAK::ActionRegistry *actionRegistry();

        Q_INVOKABLE static int execSettingsDialog(const QString &id, QWindow *parent);
        Q_INVOKABLE static void execPluginsDialog(QWindow *parent);
        Q_INVOKABLE static void execAboutAppDialog(QWindow *parent);
        Q_INVOKABLE static void execAboutQtDialog(QWindow *parent);
        Q_INVOKABLE static void showHome();
        Q_INVOKABLE static void checkForUpdate(bool silent = false);

    public:
        Q_INVOKABLE static QQuickWindow *newFile();
        static bool openFile(const QString &fileName, QWidget *parent = nullptr);

    Q_SIGNALS:
        void resetAllDoNotShowAgainRequested();

    private:
        explicit ICore(QObject *parent = nullptr);
        ~ICore();

        ICore(ICorePrivate &d, QObject *parent = nullptr);

        friend class Internal::CorePlugin;
    };

}

#endif // DIFFSCOPE_COREPLUGIN_ICORE_H