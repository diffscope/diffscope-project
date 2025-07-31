#include "iprojectwindow.h"

#include <QQmlComponent>

#include <coreplugin/icore.h>
#include <coreplugin/internal/projectwindowdata.h>

namespace Core {

    static IProjectWindow *m_instance = nullptr;

    class IProjectWindowPrivate {
        Q_DECLARE_PUBLIC(IProjectWindow)
    public:
        IProjectWindow *q_ptr;
        Internal::ProjectWindowData *windowData;
        void init() {
            Q_Q(IProjectWindow);
            windowData = new Internal::ProjectWindowData(q);
            ICore::actionRegistry()->addContext(windowData->actionContext());
        }
    };

    IProjectWindow *IProjectWindow::instance() {
        return m_instance;
    }
    QAK::QuickActionContext *IProjectWindow::actionContext() const {
        Q_D(const IProjectWindow);
        return d->windowData->actionContext();
    }
    QWindow *IProjectWindow::createWindow(QObject *parent) const {
        Q_D(const IProjectWindow);
        QQmlComponent component(ICore::qmlEngine(), "DiffScope.CorePlugin", "ProjectWindow");
        if (component.isError()) {
            qFatal() << component.errorString();
        }
        auto win = qobject_cast<QWindow *>(component.createWithInitialProperties({{"windowData", QVariant::fromValue(d->windowData)}}));
        Q_ASSERT(win);
        d->windowData->setParent(win);
        return win;
    }
    IProjectWindow::IProjectWindow(QObject *parent) : IProjectWindow(*new IProjectWindowPrivate, parent) {
        m_instance = this;
    }
    IProjectWindow::IProjectWindow(IProjectWindowPrivate &d, QObject *parent) : IWindow(parent), d_ptr(&d) {
        d.q_ptr = this;
        d.init();
    }
    IProjectWindow::~IProjectWindow() {
        m_instance = nullptr;
    }
    void IProjectWindow::nextLoadingState(State nextState) {
        Q_D(IProjectWindow);
        if (nextState == Initialized) {
            d->windowData->actionContext()->updateElement(QAK::AE_Layouts);
            emit d->windowData->initialized();
        }
    }
    IProjectWindowRegistry *IProjectWindowRegistry::instance() {
        static IProjectWindowRegistry reg;
        return &reg;
    }
}
