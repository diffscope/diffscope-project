#include "coreplugin.h"

#include <QtCore/QDirIterator>
#include <QtCore/QLoggingCategory>
#include <QtCore/QThread>
#include <QtGui/QFileOpenEvent>
#include <QtWidgets/QApplication>
#include <QtWidgets/QSplashScreen>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QPushButton>

#include <extensionsystem/pluginspec.h>

#include <CoreApi/plugindatabase.h>

#include <loadapi/initroutine.h>

#include "icore.h"

namespace Core::Internal {

    static ICore *icore = nullptr;

    static void waitSplash(QWidget *w) {
        PluginDatabase::splash()->finish(w);
    }

    CorePlugin::CorePlugin() {
    }

    CorePlugin::~CorePlugin() {
    }

    bool CorePlugin::initialize(const QStringList &arguments, QString *errorMessage) {
        PluginDatabase::splash()->showMessage(tr("Initializing core plugin..."));

        // qApp->setWindowIcon(QIcon(":/svg/app/diffsinger.svg"));

        // Init ICore instance
        icore = new ICore(this);

        // Handle FileOpenEvent
        qApp->installEventFilter(this);

        return true;
    }

    void CorePlugin::extensionsInitialized() {
    }

    bool CorePlugin::delayedInitialize() {
        PluginDatabase::splash()->showMessage(tr("Initializing GUI..."));

        // if (auto entry = InitRoutine::startEntry()) {
        //     waitSplash(entry());
        //     return false;
        // }

        auto w = new QMainWindow();
        w->setAttribute(Qt::WA_DeleteOnClose);
        w->setCentralWidget([]() {
            auto button = new QPushButton(tr("Crash !!!"));
            connect(button, &QPushButton::clicked, button,
                    []() { reinterpret_cast<QString *>(0)->toInt(); });
            return button;
        }());
        w->show();
        waitSplash(w);
        return false;
    }

    QObject *CorePlugin::remoteCommand(const QStringList &options, const QString &workingDirectory,
                                       const QStringList &args) {
        // auto firstHandle = icore->windowSystem()->firstWindow();
        // int cnt = openFileFromCommand(workingDirectory, args, firstHandle);
        // if (firstHandle && cnt == 0) {
        //     QMView::raiseWindow(firstHandle->window());
        // }
        return nullptr;
    }

    bool CorePlugin::eventFilter(QObject *obj, QEvent *event) {
        if (event->type() == QEvent::FileOpen) {
            // openFileFromCommand({}, {static_cast<QFileOpenEvent *>(event)->file()},
            //                     icore->windowSystem()->firstWindow());
        }
        return QObject::eventFilter(obj, event);
    }

}
