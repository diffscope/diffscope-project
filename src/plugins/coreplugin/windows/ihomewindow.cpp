#include "ihomewindow.h"

#include <QQmlComponent>

#include <QAKQuick/quickactioncontext.h>

#include <CoreApi/plugindatabase.h>

#include <coreplugin/icore.h>
#include <coreplugin/internal/actionhelper.h>

namespace Core {

    static IHomeWindow *m_instance = nullptr;

    class IHomeWindowPrivate {
        Q_DECLARE_PUBLIC(IHomeWindow)
    public:
        IHomeWindow *q_ptr;
        QAK::QuickActionContext *actionContext;
        void init() {
            Q_Q(IHomeWindow);
            actionContext = new QAK::QuickActionContext(q);
            initActionContext();
            ICore::actionRegistry()->addContext(actionContext);
        }
        void initActionContext() {
            Q_Q(IHomeWindow);
            actionContext->setMenuComponent(new QQmlComponent(PluginDatabase::qmlEngine(), "SVSCraft.UIComponents", "Menu", q));
            actionContext->setSeparatorComponent(new QQmlComponent(PluginDatabase::qmlEngine(), "SVSCraft.UIComponents", "MenuSeparator", q));
            actionContext->setStretchComponent(new QQmlComponent(PluginDatabase::qmlEngine(), "SVSCraft.UIComponents", "MenuSeparator", q));

            QQmlComponent component(PluginDatabase::qmlEngine(), "DiffScope.CorePlugin", "GlobalActions");
            if (component.isError()) {
                qFatal() << component.errorString();
            }
            auto o = component.create();
            o->setParent(q);
            QMetaObject::invokeMethod(o, "registerToContext", actionContext);
        }
    };

    IHomeWindow *IHomeWindow::instance() {
        return m_instance;
    }
    QAK::QuickActionContext *IHomeWindow::actionContext() const {
        Q_D(const IHomeWindow);
        return d->actionContext;
    }
    bool IHomeWindow::triggerAction(const QString &id, QObject *source) {
        Q_D(IHomeWindow);
        return Internal::ActionHelper::triggerAction(d->actionContext, id, source);
    }
    QWindow *IHomeWindow::createWindow(QObject *parent) const {
        Q_D(const IHomeWindow);
        QQmlComponent component(PluginDatabase::qmlEngine(), "DiffScope.CorePlugin", "HomeWindow");
        if (component.isError()) {
            qFatal() << component.errorString();
        }
        auto win = qobject_cast<QWindow *>(component.createWithInitialProperties({{"windowHandle", QVariant::fromValue(this)}}));
        Q_ASSERT(win);
        return win;
    }
    IHomeWindow::IHomeWindow(QObject *parent) : IHomeWindow(*new IHomeWindowPrivate, parent) {
        m_instance = this;
    }
    IHomeWindow::IHomeWindow(IHomeWindowPrivate &d, QObject *parent) : IWindow(parent), d_ptr(&d) {
        d.q_ptr = this;
        d.init();
    }
    IHomeWindow::~IHomeWindow() {
        m_instance = nullptr;
    }
    void IHomeWindow::nextLoadingState(State nextState) {
        Q_D(IHomeWindow);
        if (nextState == Initialized) {
            d->actionContext->updateElement(QAK::AE_Layouts);
        }
    }
    IHomeWindowRegistry *IHomeWindowRegistry::instance() {
        static IHomeWindowRegistry reg;
        return &reg;
    }
}

#include "moc_ihomewindow.cpp"
