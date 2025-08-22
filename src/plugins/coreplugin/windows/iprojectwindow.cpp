#include "iprojectwindow.h"

#include <QQmlComponent>
#include <QAbstractItemModel>

#include <QAKQuick/quickactioncontext.h>

#include <CoreApi/plugindatabase.h>

#include <coreplugin/icore.h>
#include <coreplugin/notificationmessage.h>
#include <coreplugin/internal/notificationmanager.h>
#include <coreplugin/quickpick.h>

namespace Core {

    static IProjectWindow *m_instance = nullptr;

    class IProjectWindowPrivate {
        Q_DECLARE_PUBLIC(IProjectWindow)
    public:
        IProjectWindow *q_ptr;
        QAK::QuickActionContext *actionContext;
        Internal::NotificationManager *notificationManager;
        void init() {
            Q_Q(IProjectWindow);
            actionContext = new QAK::QuickActionContext(q);
            initActionContext();
            ICore::actionRegistry()->addContext(actionContext);
            notificationManager = new Internal::NotificationManager(q);
        }

        void initActionContext() {
            Q_Q(IProjectWindow);
            actionContext->setMenuComponent(new QQmlComponent(PluginDatabase::qmlEngine(), "SVSCraft.UIComponents", "Menu", q));
            actionContext->setSeparatorComponent(new QQmlComponent(PluginDatabase::qmlEngine(), "SVSCraft.UIComponents", "MenuSeparator", q));
            actionContext->setStretchComponent(new QQmlComponent(PluginDatabase::qmlEngine(), "SVSCraft.UIComponents", "MenuSeparator", q));
            {
                QQmlComponent component(PluginDatabase::qmlEngine(), "DiffScope.CorePlugin", "GlobalActions");
                if (component.isError()) {
                    qFatal() << component.errorString();
                }
                auto o = component.create();
                o->setParent(q);
                QMetaObject::invokeMethod(o, "registerToContext", actionContext);
            }
            {
                QQmlComponent component(PluginDatabase::qmlEngine(), "DiffScope.CorePlugin", "ProjectActions");
                if (component.isError()) {
                    qFatal() << component.errorString();
                }
                auto o = component.createWithInitialProperties({
                    {"windowHandle", QVariant::fromValue(q)},
                });
                o->setParent(q);
                QMetaObject::invokeMethod(o, "registerToContext", actionContext);
            }
        }
    };

    IProjectWindow *IProjectWindow::instance() {
        return m_instance;
    }
    QAK::QuickActionContext *IProjectWindow::actionContext() const {
        Q_D(const IProjectWindow);
        return d->actionContext;
    }
    void IProjectWindow::sendNotification(NotificationMessage *message, NotificationBubbleMode mode) {
        Q_D(IProjectWindow);
        d->notificationManager->addMessage(message, mode);
    }
    void IProjectWindow::sendNotification(SVS::SVSCraft::MessageBoxIcon icon, const QString &title,
                                          const QString &text, NotificationBubbleMode mode) {
        auto message = new NotificationMessage(this);
        message->setIcon(icon);
        message->setTitle(title);
        message->setText(text);
        connect(message, &NotificationMessage::closed, message, &QObject::deleteLater);
        sendNotification(message, mode);
    }
    int IProjectWindow::execQuickPick(QAbstractItemModel *model, int defaultIndex, const QString &initialFilterText, const QString &placeholderText) {
        QuickPick quickPick;
        quickPick.setWindowHandle(this);
        quickPick.setModel(model);
        quickPick.setPlaceholderText(placeholderText);
        quickPick.setFilterText(initialFilterText);
        quickPick.setCurrentIndex(defaultIndex);
        return quickPick.exec();
    }
    QWindow *IProjectWindow::createWindow(QObject *parent) const {
        Q_D(const IProjectWindow);
        QQmlComponent component(PluginDatabase::qmlEngine(), "DiffScope.CorePlugin", "ProjectWindow");
        if (component.isError()) {
            qFatal() << component.errorString();
        }
        auto win = qobject_cast<QWindow *>(component.createWithInitialProperties({
            {"windowHandle", QVariant::fromValue(this)}
        }));
        Q_ASSERT(win);
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
            d->actionContext->updateElement(QAK::AE_Layouts);
        }
    }
    IProjectWindowRegistry *IProjectWindowRegistry::instance() {
        static IProjectWindowRegistry reg;
        return &reg;
    }
}

#include "moc_iprojectwindow.cpp"
