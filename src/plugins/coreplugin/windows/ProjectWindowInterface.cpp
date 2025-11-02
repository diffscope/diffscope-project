#include "ProjectWindowInterface.h"

#include <QAbstractItemModel>
#include <QEventLoop>
#include <QFileDialog>
#include <QFileInfo>
#include <QJSValue>
#include <QLoggingCategory>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickWindow>
#include <QStandardPaths>

#include <CoreApi/filelocker.h>
#include <CoreApi/recentfilecollection.h>
#include <CoreApi/runtimeinterface.h>

#include <QAKQuick/quickactioncontext.h>

#include <SVSCraftQuick/MessageBox.h>
#include <SVSCraftQuick/StatusTextContext.h>

#include <coreplugin/CoreInterface.h>
#include <coreplugin/EditActionsHandlerRegistry.h>
#include <coreplugin/internal/ActionHelper.h>
#include <coreplugin/internal/BehaviorPreference.h>
#include <coreplugin/internal/NotificationManager.h>
#include <coreplugin/NotificationMessage.h>
#include <coreplugin/ProjectDocumentContext.h>
#include <coreplugin/ProjectTimeline.h>
#include <coreplugin/ProjectViewModelContext.h>
#include <coreplugin/QuickInput.h>
#include <coreplugin/QuickPick.h>

namespace Core {

    Q_LOGGING_CATEGORY(lcProjectWindow, "diffscope.core.projectwindow")

    static ProjectWindowInterface *m_instance = nullptr;

    class MessageBoxDialogDoneListener : public QObject {
        Q_OBJECT
    public:
        inline explicit MessageBoxDialogDoneListener(QEventLoop *eventLoop) : eventLoop(eventLoop) {
        }

    public slots:
        void done(const QVariant &id) const {
            eventLoop->exit(id.toInt());
        }

    private:
        QEventLoop *eventLoop;
    };

    class ProjectWindowInterfacePrivate {
        Q_DECLARE_PUBLIC(ProjectWindowInterface)
    public:
        ProjectWindowInterface *q_ptr;
        Internal::NotificationManager *notificationManager;
        ProjectTimeline *projectTimeline;
        EditActionsHandlerRegistry *mainEditActionsHandlerRegistry;
        ProjectDocumentContext *projectDocumentContext;
        ProjectViewModelContext *projectViewModelContext;
        void init() {
            Q_Q(ProjectWindowInterface);
            initActionContext();
            notificationManager = new Internal::NotificationManager(q);
            projectTimeline = new ProjectTimeline(q);
            mainEditActionsHandlerRegistry = new EditActionsHandlerRegistry(q);
            projectViewModelContext = new ProjectViewModelContext(projectTimeline, q);
        }

        void initActionContext() {
            Q_Q(ProjectWindowInterface);
            auto actionContext = q->actionContext();
            QQmlComponent component(RuntimeInterface::qmlEngine(), "DiffScope.Core", "ProjectActions");
            if (component.isError()) {
                qFatal() << component.errorString();
            }
            auto o = component.createWithInitialProperties({
                {"windowHandle", QVariant::fromValue(q)},
            });
            o->setParent(q);
            QMetaObject::invokeMethod(o, "registerToContext", actionContext);
        }

        QString promptSaveDspxFile() const {
            auto settings = RuntimeInterface::settings();
            settings->beginGroup(ProjectWindowInterface::staticMetaObject.className());
            auto defaultSaveDir = projectDocumentContext->fileLocker() && !projectDocumentContext->fileLocker()->path().isEmpty() ? projectDocumentContext->fileLocker()->path() : settings->value(QStringLiteral("defaultSaveDir"), QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)).toString();
            settings->endGroup();

            auto path = QFileDialog::getSaveFileName(
                nullptr,
                {},
                defaultSaveDir,
                CoreInterface::dspxFileFilter(true)
            );
            if (path.isEmpty())
                return {};

            settings->beginGroup(ProjectWindowInterface::staticMetaObject.className());
            settings->setValue(QStringLiteral("defaultSaveDir"), QFileInfo(path).absolutePath());
            settings->endGroup();

            return path;
        }

        void updateRecentFile() const {
            Q_Q(const ProjectWindowInterface);
            auto win = q->window();
            auto pixmap = win->screen()->grabWindow(win->winId());
            CoreInterface::recentFileCollection()->addRecentFile(projectDocumentContext->fileLocker()->path(), pixmap);
        }

        enum ExternalChangeOperation {
            SaveAs,
            Overwrite,
            Cancel = SVS::SVSCraft::Cancel
        };

        ExternalChangeOperation promptFileExternalChange() const {
            Q_Q(const ProjectWindowInterface);
            QQmlComponent component(RuntimeInterface::qmlEngine(), "SVSCraft.UIComponents", "MessageBoxDialog");
            std::unique_ptr<QQuickWindow> mb(qobject_cast<QQuickWindow *>(component.createWithInitialProperties(
                {{"text", Core::ProjectWindowInterface::tr("File Modified Externally")},
                 {"informativeText", Core::ProjectWindowInterface::tr("The file has been modified by another program since it was last saved.\n\nDo you want to save as a new file or overwrite it?")},
                 {"buttons", QVariantList{
                                 QVariantMap{
                                     {"id", SaveAs},
                                     {"text", Core::ProjectWindowInterface::tr("Save As...")},
                                 },
                                 QVariantMap{
                                     {"id", Overwrite},
                                     {"text", Core::ProjectWindowInterface::tr("Overwrite")},
                                 },
                                 SVS::SVSCraft::Cancel,
                             }},
                 {"primaryButton", SaveAs},
                 {"icon", SVS::SVSCraft::Warning},
                 {"transientParent", QVariant::fromValue(q->window())}}
            )));
            Q_ASSERT(mb);
            QEventLoop eventLoop;
            MessageBoxDialogDoneListener listener(&eventLoop);
            QObject::connect(mb.get(), SIGNAL(done(QVariant)), &listener, SLOT(done(QVariant)));
            mb->show();
            return static_cast<ExternalChangeOperation>(eventLoop.exec());
        }
    };

    ProjectWindowInterface *ProjectWindowInterface::instance() {
        return m_instance;
    }

    ProjectTimeline *ProjectWindowInterface::projectTimeline() const {
        Q_D(const ProjectWindowInterface);
        return d->projectTimeline;
    }

    ProjectDocumentContext *ProjectWindowInterface::projectDocumentContext() const {
        Q_D(const ProjectWindowInterface);
        return d->projectDocumentContext;
    }

    EditActionsHandlerRegistry *ProjectWindowInterface::mainEditActionsHandlerRegistry() const {
        Q_D(const ProjectWindowInterface);
        return d->mainEditActionsHandlerRegistry;
    }

    ProjectViewModelContext *ProjectWindowInterface::projectViewModelContext() const {
        Q_D(const ProjectWindowInterface);
        return d->projectViewModelContext;
    }

    void ProjectWindowInterface::sendNotification(NotificationMessage *message, NotificationBubbleMode mode) {
        Q_D(ProjectWindowInterface);
        d->notificationManager->addMessage(message, mode);
    }
    void ProjectWindowInterface::sendNotification(SVS::SVSCraft::MessageBoxIcon icon, const QString &title, const QString &text, NotificationBubbleMode mode) {
        auto message = new NotificationMessage(this);
        message->setIcon(icon);
        message->setTitle(title);
        message->setText(text);
        connect(message, &NotificationMessage::closed, message, &QObject::deleteLater);
        sendNotification(message, mode);
    }

    bool ProjectWindowInterface::save() {
        Q_D(ProjectWindowInterface);
        if (!d->projectDocumentContext->fileLocker() || d->projectDocumentContext->fileLocker()->path().isEmpty())
            return saveAs();
        if (d->projectDocumentContext->fileLocker()->isFileModifiedSinceLastSave() && (Internal::BehaviorPreference::fileOption() & Internal::BehaviorPreference::FO_CheckForExternalChangedOnSave)) {
            auto op = d->promptFileExternalChange();
            if (op == ProjectWindowInterfacePrivate::Cancel) {
                return false;
            }
            if (op == ProjectWindowInterfacePrivate::SaveAs) {
                return saveAs();
            }
        }
        bool isSuccess = d->projectDocumentContext->save(window());
        if (isSuccess) {
            d->updateRecentFile();
            return true;
        }
        return false;
    }

    bool ProjectWindowInterface::saveAs() {
        Q_D(ProjectWindowInterface);
        auto path = d->promptSaveDspxFile();
        if (path.isEmpty())
            return false;
        bool isSuccess = d->projectDocumentContext->saveAs(path, window());
        if (isSuccess) {
            d->updateRecentFile();
            return true;
        }
        return false;
    }

    bool ProjectWindowInterface::saveCopy() {
        Q_D(ProjectWindowInterface);
        auto path = d->promptSaveDspxFile();
        if (path.isEmpty())
            return false;
        return d->projectDocumentContext->saveCopy(path, window());
    }

    QWindow *ProjectWindowInterface::createWindow(QObject *parent) const {
        Q_D(const ProjectWindowInterface);
        QQmlComponent component(RuntimeInterface::qmlEngine(), "DiffScope.Core", "ProjectWindow");
        if (component.isError()) {
            qFatal() << component.errorString();
        }
        auto win = qobject_cast<QQuickWindow *>(component.createWithInitialProperties({{"windowHandle", QVariant::fromValue(this)}
        }));
        Q_ASSERT(win);
        SVS::StatusTextContext::setStatusContext(win, new SVS::StatusTextContext(win));
        SVS::StatusTextContext::setContextHelpContext(win, new SVS::StatusTextContext(win));
        return win;
    }
    ProjectWindowInterface::ProjectWindowInterface(ProjectDocumentContext *projectDocumentContext, QObject *parent) : ProjectWindowInterface(*new ProjectWindowInterfacePrivate, parent) {
        Q_D(ProjectWindowInterface);
        m_instance = this;
        d->projectDocumentContext = projectDocumentContext;
    }
    ProjectWindowInterface::ProjectWindowInterface(ProjectWindowInterfacePrivate &d, QObject *parent) : ActionWindowInterfaceBase(parent), d_ptr(&d) {
        d.q_ptr = this;
        d.init();
    }
    ProjectWindowInterface::~ProjectWindowInterface() {
        m_instance = nullptr;
    }

    ProjectWindowInterfaceRegistry *ProjectWindowInterfaceRegistry::instance() {
        static ProjectWindowInterfaceRegistry reg;
        return &reg;
    }
}

#include "moc_ProjectWindowInterface.cpp"
#include "ProjectWindowInterface.moc"
