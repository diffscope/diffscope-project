#include "ihomewindow.h"

#include <QQmlComponent>

#include <coreplugin/icore.h>
#include <coreplugin/internal/homewindowdata.h>

namespace Core {

    static IHomeWindow *m_instance = nullptr;

    class IHomeWindowPrivate {
        Q_DECLARE_PUBLIC(IHomeWindow)
    public:
        IHomeWindow *q_ptr;
        Internal::HomeWindowData *windowData;
        void init() {
            Q_Q(IHomeWindow);
            windowData = new Internal::HomeWindowData(q);
            ICore::actionRegistry()->addContext(windowData->actionContext());
        }
    };

    IHomeWindow *IHomeWindow::instance() {
        return m_instance;
    }
    QAK::QuickActionContext *IHomeWindow::actionContext() const {
        Q_D(const IHomeWindow);
        return d->windowData->actionContext();
    }
    QWindow *IHomeWindow::createWindow(QObject *parent) const {
        Q_D(const IHomeWindow);
        QQmlComponent component(ICore::qmlEngine(), "DiffScope.CorePlugin", "HomeWindow");
        if (component.isError()) {
            qFatal() << component.errorString();
        }
        auto win = qobject_cast<QWindow *>(component.createWithInitialProperties({{"windowData", QVariant::fromValue(d->windowData)}}));
        Q_ASSERT(win);
        d->windowData->setParent(win);
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
            d->windowData->actionContext()->updateElement(QAK::AE_Layouts);
        }
    }
    IHomeWindowRegistry *IHomeWindowRegistry::instance() {
        static IHomeWindowRegistry reg;
        return &reg;
    }
}
