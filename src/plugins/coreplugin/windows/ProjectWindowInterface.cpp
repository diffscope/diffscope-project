#include "ProjectWindowInterface.h"

#include <QAbstractItemModel>
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

#include <SVSCraftCore/MusicTime.h>
#include <SVSCraftCore/MusicTimeline.h>
#include <SVSCraftQuick/MessageBox.h>
#include <SVSCraftQuick/StatusTextContext.h>

#include <dspxmodel/Clip.h>
#include <dspxmodel/ClipSequence.h>
#include <dspxmodel/ClipTime.h>
#include <dspxmodel/Label.h>
#include <dspxmodel/LabelSequence.h>
#include <dspxmodel/Model.h>
#include <dspxmodel/Note.h>
#include <dspxmodel/NoteSequence.h>
#include <dspxmodel/SingingClip.h>
#include <dspxmodel/Tempo.h>
#include <dspxmodel/TempoSequence.h>
#include <dspxmodel/TimeSignature.h>
#include <dspxmodel/TimeSignatureSequence.h>
#include <dspxmodel/Timeline.h>
#include <dspxmodel/Track.h>
#include <dspxmodel/TrackList.h>

#include <coreplugin/CoreInterface.h>
#include <coreplugin/DspxDocument.h>
#include <coreplugin/NotificationMessage.h>
#include <coreplugin/OpenSaveProjectFileScenario.h>
#include <coreplugin/ProjectDocumentContext.h>
#include <coreplugin/ProjectTimeline.h>
#include <coreplugin/QuickInput.h>
#include <coreplugin/QuickPick.h>
#include <coreplugin/internal/ActionHelper.h>
#include <coreplugin/internal/BehaviorPreference.h>
#include <coreplugin/internal/NotificationManager.h>

namespace Core {

    Q_LOGGING_CATEGORY(lcProjectWindow, "diffscope.core.projectwindow")

    class ProjectWindowInterfacePrivate {
        Q_DECLARE_PUBLIC(ProjectWindowInterface)
    public:
        ProjectWindowInterface *q_ptr;
        Internal::NotificationManager *notificationManager;
        ProjectTimeline *projectTimeline;
        ProjectDocumentContext *projectDocumentContext;

        void init() {
            Q_Q(ProjectWindowInterface);
            initActionContext();
            notificationManager = new Internal::NotificationManager(q);
            projectTimeline = new ProjectTimeline(projectDocumentContext->document(), q);
            q->boundTimelineRangeHint();
        }

        void initActionContext() {
            Q_Q(ProjectWindowInterface);
            auto actionContext = q->actionContext();
            {
                QQmlComponent component(RuntimeInterface::qmlEngine(), "DiffScope.Core", "GlobalActions");
                if (component.isError()) {
                    qFatal() << component.errorString();
                }
                auto o = component.createWithInitialProperties({
                    {"windowHandle", QVariant::fromValue(q)}
                });
                o->setParent(q);
                QMetaObject::invokeMethod(o, "registerToContext", actionContext);
            }
            {
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
            return static_cast<ExternalChangeOperation>(SVS::MessageBox::customExec(mb.get()).toInt());
        }
    };

    ProjectTimeline *ProjectWindowInterface::projectTimeline() const {
        Q_D(const ProjectWindowInterface);
        return d->projectTimeline;
    }

    ProjectDocumentContext *ProjectWindowInterface::projectDocumentContext() const {
        Q_D(const ProjectWindowInterface);
        return d->projectDocumentContext;
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
        bool isSuccess = d->projectDocumentContext->save();
        if (isSuccess) {
            d->updateRecentFile();
            return true;
        }
        return false;
    }

    bool ProjectWindowInterface::saveAs() {
        Q_D(ProjectWindowInterface);
        auto path = d->projectDocumentContext->openSaveProjectFileScenario()->saveProjectFile(d->projectDocumentContext->fileLocker() ? d->projectDocumentContext->fileLocker()->path() : QString());
        if (path.isEmpty())
            return false;
        bool isSuccess = d->projectDocumentContext->saveAs(path);
        if (isSuccess) {
            d->updateRecentFile();
            return true;
        }
        return false;
    }

    bool ProjectWindowInterface::saveCopy() {
        Q_D(ProjectWindowInterface);
        auto path = d->projectDocumentContext->openSaveProjectFileScenario()->saveProjectFile(d->projectDocumentContext->fileLocker() ? d->projectDocumentContext->fileLocker()->path() : QString());
        if (path.isEmpty())
            return false;
        return d->projectDocumentContext->saveCopy(path);
    }

    void ProjectWindowInterface::boundTimelineRangeHint() {
        Q_D(ProjectWindowInterface);
        auto model = d->projectDocumentContext->document()->model();
        d->projectTimeline->setRangeHint(1920 + std::max({
            model->timeline()->loopStart() + model->timeline()->loopLength(),
            model->timeline()->labels()->size() == 0 ? 0 : model->timeline()->labels()->lastItem()->pos(),
            model->timeline()->tempos()->size() == 0 ? 0 : model->timeline()->tempos()->lastItem()->pos(),
            model->timeline()->timeSignatures()->size() == 0 ? 0 : d->projectTimeline->musicTimeline()->create(model->timeline()->timeSignatures()->lastItem()->index(), 0, 0).totalTick(),
            model->tracks()->size() == 0 ? 0 : std::ranges::max(std::views::transform(model->tracks()->items(), [](dspx::Track *track) {
                return track->clips()->size() == 0 ? 0 : std::ranges::max(std::views::transform(track->clips()->asRange(), [](dspx::Clip *clip) {
                    return std::max({
                        clip->time()->start() + clip->time()->clipStart() + clip->time()->clipLen(),
                        clip->type() == dspx::Clip::Audio ? 0 : clip->time()->start() + static_cast<dspx::SingingClip *>(clip)->notes()->size() == 0 ? 0 : std::ranges::max(std::views::transform(static_cast<dspx::SingingClip *>(clip)->notes()->asRange(), [](dspx::Note *note) {
                            return note->pos() + note->length();
                        })),
                        // TODO param
                    });
                }));
            }))
        }));
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
        d->projectDocumentContext->openSaveProjectFileScenario()->setWindow(win);
        static QIcon dspxIcon = [] {
            QIcon icon;
            for (const auto &file : QDir(":/diffscope/icons/dspx").entryInfoList(QDir::Files)) {
                icon.addFile(file.absoluteFilePath());
            }
            return icon;
        }();
        win->setIcon(dspxIcon);
        QString path;
        if (d->projectDocumentContext->fileLocker() && !d->projectDocumentContext->fileLocker()->path().isEmpty()) {
            path = d->projectDocumentContext->fileLocker()->path();
        } else {
            path = tr("Untitled") + ".dspx";
        }
        win->setFilePath(path);
        return win;
    }

    ProjectWindowInterface::ProjectWindowInterface(ProjectDocumentContext *projectDocumentContext, QObject *parent) : ProjectWindowInterface(*new ProjectWindowInterfacePrivate, parent) {
        Q_D(ProjectWindowInterface);
        d->projectDocumentContext = projectDocumentContext;
        if (d->projectDocumentContext->fileLocker()) {
            connect(d->projectDocumentContext->fileLocker(), &FileLocker::pathChanged, this, [=, this] {
                auto win = window();
                if (!win)
                    return;
                auto path = d->projectDocumentContext->fileLocker()->path();
                win->setFilePath(path.isEmpty() ? tr("Untitled") + ".dspx" : path);
            });
        }
        d->init();
    }
    ProjectWindowInterface::ProjectWindowInterface(ProjectWindowInterfacePrivate &d, QObject *parent) : ActionWindowInterfaceBase(parent), d_ptr(&d) {
        d.q_ptr = this;
    }
    ProjectWindowInterface::~ProjectWindowInterface() = default;

    ProjectWindowInterfaceRegistry *ProjectWindowInterfaceRegistry::instance() {
        static ProjectWindowInterfaceRegistry reg;
        return &reg;
    }
}

#include "moc_ProjectWindowInterface.cpp"
#include "ProjectWindowInterface.moc"
