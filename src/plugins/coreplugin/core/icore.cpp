#include "icore.h"

#include <csignal>
#include <memory>

#include "QStyleFactory"
#include <QApplication>
#include <QChildEvent>
#include <QMessageBox>
#include <QScreen>
#include <QScrollArea>
#include <QScrollBar>
#include <QTimer>
#include <QToolBar>
#include <QToolButton>
#include <QGridLayout>
#include <QQmlEngine>
#include <QQmlComponent>

#include <extensionsystem/pluginmanager.h>

#include <CoreApi/private/icorebase_p.h>

namespace Core {

    class ICorePrivate : ICoreBasePrivate {
        Q_DECLARE_PUBLIC(ICore)
    public:
        ICorePrivate() {
        }

        QQmlEngine *qmlEngine;

        void init() {
            Q_Q(ICore);
            qmlEngine = new QQmlEngine(q);
        }
    };

    ICore *ICore::instance() {
        return static_cast<ICore *>(ICoreBase::instance());
    }
    QQmlEngine *ICore::qmlEngine() {
        if (!instance())
            return nullptr;
        return instance()->d_func()->qmlEngine;
    }

    int ICore::showSettingsDialog(const QString &id, QWindow *parent) {
        static std::unique_ptr<QWindow> dlg;

        if (dlg) {
            if (!id.isEmpty())
                QMetaObject::invokeMethod(dlg.get(), "showPage", QVariant(id));
            return -1;
        }

        int code;
        {
            QQmlComponent component(qmlEngine(), "DiffScope.CorePlugin", "SettingDialog");
            if (component.isError()) {
                qFatal() << component.errorString();
            }
            dlg.reset(qobject_cast<QWindow *>(component.create()));
            Q_ASSERT(dlg);
            dlg->setTransientParent(parent);
            if (!id.isEmpty())
                QMetaObject::invokeMethod(dlg.get(), "showPage", QVariant(id));
            dlg->show();
            QEventLoop eventLoop;
            connect(dlg.get(), SIGNAL(finished()), &eventLoop, SLOT(quit()));
            eventLoop.exec();
            dlg.reset();
        }

        // return code;
        return 0;
    }

    void ICore::showPluginsDialog(QWindow *parent) {
        QQmlComponent component(qmlEngine(), "DiffScope.CorePlugin", "PluginDialog");
        if (component.isError()) {
            qFatal() << component.errorString();
        }
        std::unique_ptr<QWindow> dlg(qobject_cast<QWindow *>(component.create()));
        Q_ASSERT(dlg);
        dlg->setTransientParent(parent);
        dlg->show();
        QEventLoop eventLoop;
        connect(dlg.get(), SIGNAL(finished()), &eventLoop, SLOT(quit()));
        eventLoop.exec();
    }

    void ICore::showHome() {
        // auto inst = IHomeWindow::instance();
        // if (inst) {
        //     QMView::raiseWindow(inst->window());
        //     return;
        // }
        // IHomeWindowRegistry::instance()->create();
    }
    void ICore::newFile() const {
    }

    bool ICore::openFile(const QString &fileName, QWidget *parent) const {
        // auto docMgr = ICore::instance()->documentSystem();
        // if (fileName.isEmpty()) {
        //     return docMgr->openFileBrowse(parent, DspxSpec::instance());
        // }
        // return DspxSpec::instance()->open(fileName, parent);
        return false;
    }

    ICore::ICore(QObject *parent) : ICore(*new ICorePrivate(), parent) {
    }

    ICore::~ICore() {
    }

    ICore::ICore(ICorePrivate &d, QObject *parent) : ICoreBase(d, parent) {
        d.init();
    }


}