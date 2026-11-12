// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#include "ProjectWindowInterface.h"
#include "ProjectWindowInterface_p.h"

#include <algorithm>
#include <ranges>
#include <utility>

#include <QColor>
#include <QDir>
#include <QIcon>
#include <QPointer>
#include <QQmlComponent>
#include <QQuickWindow>
#include <QVariant>

#include <CoreApi/filelocker.h>
#include <CoreApi/runtimeinterface.h>

#include <QAKQuick/quickactioncontext.h>

#include <SVSCraftCore/MusicTime.h>
#include <SVSCraftCore/MusicTimeline.h>
#include <SVSCraftQuick/StatusTextContext.h>

#include <dspxmodelORM/Clip.h>
#include <dspxmodelORM/ClipSequence.h>
#include <dspxmodelORM/Label.h>
#include <dspxmodelORM/LabelSequence.h>
#include <dspxmodelORM/Model.h>
#include <dspxmodelORM/Note.h>
#include <dspxmodelORM/NoteSequence.h>
#include <dspxmodelORM/SingingClip.h>
#include <dspxmodelORM/Tempo.h>
#include <dspxmodelORM/TempoSequence.h>
#include <dspxmodelORM/TimeSignature.h>
#include <dspxmodelORM/TimeSignatureSequence.h>
#include <dspxmodelORM/Track.h>
#include <dspxmodelORM/TrackList.h>

#include <dspxproject/Clipboard.h>
#include <dspxproject/ClipboardEditScenario.h>
#include <dspxproject/Document.h>
#include <dspxproject/EditItemsScenario.h>
#include <dspxproject/EditKeySignatureScenario.h>
#include <dspxproject/EditLoopScenario.h>
#include <dspxproject/EditSourcesScenario.h>
#include <dspxproject/EditTempoTimeSignatureScenario.h>
#include <dspxproject/FillLyricsScenario.h>
#include <dspxproject/InsertItemScenario.h>
#include <dspxproject/ParameterResamplingScenario.h>
#include <dspxproject/PickTrackColorScenario.h>
#include <dspxproject/ProjectDocumentContext.h>
#include <dspxproject/ProjectTimeline.h>

#include <notifications/NotificationCenter.h>
#include <notifications/NotificationMessage.h>
#include <notifications/NotificationModel.h>

#include <coreplugin/CoreInterface.h>
#include <coreplugin/TrackColorSchema.h>
#include <coreplugin/WorkspacePanelRegistry.h>
#include <coreplugin/internal/BehaviorPreference.h>
#include <coreplugin/internal/CoreClipboardEditStrategy.h>
#include <coreplugin/internal/CoreEditItemsStrategy.h>
#include <coreplugin/internal/CoreEditKeySignatureStrategy.h>
#include <coreplugin/internal/CoreEditLoopStrategy.h>
#include <coreplugin/internal/CoreEditSourcesStrategy.h>
#include <coreplugin/internal/CoreEditTempoTimeSignatureStrategy.h>
#include <coreplugin/internal/CoreFillLyricsStrategy.h>
#include <coreplugin/internal/CoreInsertItemStrategy.h>
#include <coreplugin/internal/CoreParameterResamplingStrategy.h>
#include <coreplugin/internal/CorePickTrackColorStrategy.h>
#include <singercatalog/DefaultLyricManager.h>

namespace Core {

    template <typename Scenario, typename Strategy, typename... Args>
    static Scenario *createEditScenario(ProjectWindowInterface *projectWindow, DspxProject::Document *document, Args &&...args) {
        auto strategy = new Strategy(document);
        auto scenario = new Scenario(document, strategy, std::forward<Args>(args)..., projectWindow);
        strategy->setParent(scenario);
        return scenario;
    }

    void ProjectWindowInterfacePrivate::init() {
        Q_Q(ProjectWindowInterface);
        auto document = projectDocumentContext->document();
        Q_ASSERT(!ProjectWindowInterface::of(document));
        notifications = RuntimeInterface::instance()->getFirstObject<Notifications::NotificationCenter>()->notificationsForScope(q);
        projectTimeline = new DspxProject::ProjectTimeline(document, q);
        workspacePanelRegistry = new WorkspacePanelRegistry(q);
        q->addObject(workspacePanelRegistry);
        document->setProperty(ProjectWindowInterface::staticMetaObject.className(), QVariant::fromValue(QPointer(q)));
        initEditScenarios();
        initActionContext();
        q->boundTimelineRangeHint();
    }

    void ProjectWindowInterfacePrivate::initEditScenarios() {
        Q_Q(ProjectWindowInterface);
        auto document = projectDocumentContext->document();
        auto resamplingStrategy = new Internal::CoreParameterResamplingStrategy(q);
        auto resampling = new DspxProject::ParameterResamplingScenario(resamplingStrategy, {
            .skipResamplingCheck = &Internal::BehaviorPreference::skipParameterResamplingCheck,
            .skipFutureResamplingChecks = [] {
                Internal::BehaviorPreference::setSkipParameterResamplingCheck(true);
                Internal::BehaviorPreference::instance()->save();
            },
        }, q);
        resamplingStrategy->setParent(resampling);
        clipboardEditScenario = createEditScenario<DspxProject::ClipboardEditScenario, Internal::CoreClipboardEditStrategy>(q, document, DspxProject::Clipboard::instance(), resampling);
        editItemsScenario = createEditScenario<DspxProject::EditItemsScenario, Internal::CoreEditItemsStrategy>(q, document, resampling);
        editKeySignatureScenario = createEditScenario<DspxProject::EditKeySignatureScenario, Internal::CoreEditKeySignatureStrategy>(q, document);
        editLoopScenario = createEditScenario<DspxProject::EditLoopScenario, Internal::CoreEditLoopStrategy>(q, document);
        editSourcesScenario = createEditScenario<DspxProject::EditSourcesScenario, Internal::CoreEditSourcesStrategy>(q, document, CoreInterface::singerRegistry());
        editTempoTimeSignatureScenario = createEditScenario<DspxProject::EditTempoTimeSignatureScenario, Internal::CoreEditTempoTimeSignatureStrategy>(q, document);
        fillLyricsScenario = createEditScenario<DspxProject::FillLyricsScenario, Internal::CoreFillLyricsStrategy>(q, document);
        insertItemScenario = createEditScenario<DspxProject::InsertItemScenario, Internal::CoreInsertItemStrategy>(q, document, DspxProject::InsertItemScenario::Options{
            .trackColorCount = [] { return static_cast<int>(CoreInterface::trackColorSchema()->colors().size()); },
            .defaultLyricForClip = [](dspx::SingingClip *clip) { return CoreInterface::defaultLyricManager()->getDefaultLyricForSingingClip(clip); },
        });
        pickTrackColorScenario = createEditScenario<DspxProject::PickTrackColorScenario, Internal::CorePickTrackColorStrategy>(q, document);
    }

    void ProjectWindowInterfacePrivate::initActionContext() {
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
            if (!o) {
                qFatal() << component.errorString();
            }
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
            if (!o) {
                qFatal() << component.errorString();
            }
            o->setParent(q);
            QMetaObject::invokeMethod(o, "registerToContext", actionContext);
        }
    }

    ProjectWindowInterface *ProjectWindowInterface::of(const DspxProject::Document *document) {
        return document->property(staticMetaObject.className()).value<QPointer<ProjectWindowInterface>>();
    }

    DspxProject::ProjectTimeline *ProjectWindowInterface::projectTimeline() const {
        Q_D(const ProjectWindowInterface);
        return d->projectTimeline;
    }

    DspxProject::ProjectDocumentContext *ProjectWindowInterface::projectDocumentContext() const {
        Q_D(const ProjectWindowInterface);
        return d->projectDocumentContext;
    }

    WorkspacePanelRegistry *ProjectWindowInterface::workspacePanelRegistry() const {
        Q_D(const ProjectWindowInterface);
        return d->workspacePanelRegistry;
    }

    DspxProject::ClipboardEditScenario *ProjectWindowInterface::clipboardEditScenario() const {
        Q_D(const ProjectWindowInterface);
        return d->clipboardEditScenario;
    }

    DspxProject::EditItemsScenario *ProjectWindowInterface::editItemsScenario() const {
        Q_D(const ProjectWindowInterface);
        return d->editItemsScenario;
    }

    DspxProject::EditKeySignatureScenario *ProjectWindowInterface::editKeySignatureScenario() const {
        Q_D(const ProjectWindowInterface);
        return d->editKeySignatureScenario;
    }

    DspxProject::EditLoopScenario *ProjectWindowInterface::editLoopScenario() const {
        Q_D(const ProjectWindowInterface);
        return d->editLoopScenario;
    }

    DspxProject::EditSourcesScenario *ProjectWindowInterface::editSourcesScenario() const {
        Q_D(const ProjectWindowInterface);
        return d->editSourcesScenario;
    }

    DspxProject::EditTempoTimeSignatureScenario *ProjectWindowInterface::editTempoTimeSignatureScenario() const {
        Q_D(const ProjectWindowInterface);
        return d->editTempoTimeSignatureScenario;
    }

    DspxProject::FillLyricsScenario *ProjectWindowInterface::fillLyricsScenario() const {
        Q_D(const ProjectWindowInterface);
        return d->fillLyricsScenario;
    }

    DspxProject::InsertItemScenario *ProjectWindowInterface::insertItemScenario() const {
        Q_D(const ProjectWindowInterface);
        return d->insertItemScenario;
    }

    DspxProject::PickTrackColorScenario *ProjectWindowInterface::pickTrackColorScenario() const {
        Q_D(const ProjectWindowInterface);
        return d->pickTrackColorScenario;
    }

    Notifications::NotificationModel *ProjectWindowInterface::notifications() const {
        Q_D(const ProjectWindowInterface);
        return d->notifications;
    }

    void ProjectWindowInterface::sendNotification(Notifications::NotificationMessage *message, NotificationBubbleMode mode) {
        Notifications::NotificationCenter::NotificationBubbleMode bubbleMode;
        switch (mode) {
            case NormalBubble:
                bubbleMode = Notifications::NotificationCenter::NormalBubble;
                break;
            case DoNotShowBubble:
                bubbleMode = Notifications::NotificationCenter::DoNotShowBubble;
                break;
            case AutoHide:
                bubbleMode = Notifications::NotificationCenter::AutoHide;
                break;
            default:
                Q_UNREACHABLE();
        }
        RuntimeInterface::instance()->getFirstObject<Notifications::NotificationCenter>()->sendNotification(message, bubbleMode, this);
    }
    void ProjectWindowInterface::sendNotification(SVS::SVSCraft::MessageBoxIcon icon, const QString &title, const QString &text, NotificationBubbleMode mode) {
        auto message = new Notifications::NotificationMessage(this);
        message->setIcon(icon);
        message->setTitle(title);
        message->setText(text);
        connect(message, &Notifications::NotificationMessage::closed, message, &QObject::deleteLater);
        sendNotification(message, mode);
    }

    void ProjectWindowInterface::boundTimelineRangeHint() {
        Q_D(ProjectWindowInterface);
        auto model = d->projectDocumentContext->document()->model();
        d->projectTimeline->setRangeHint(1920 + std::max({
            model->loopStart() + model->loopLength(),
            model->labels()->size() == 0 ? 0 : model->labels()->lastItem()->position(),
            model->tempos()->size() == 0 ? 0 : model->tempos()->lastItem()->position(),
            model->timeSignatures()->size() == 0 ? 0 : d->projectTimeline->musicTimeline()->create(model->timeSignatures()->lastItem()->index(), 0, 0).totalTick(),
            model->tracks()->size() == 0 ? 0 : std::ranges::max(std::views::transform(model->tracks()->items(), [](dspx::Track *track) {
                return track->clips()->size() == 0 ? 0 : std::ranges::max(std::views::transform(track->clips()->asRange(), [](dspx::Clip *clip) {
                    const int clipRight = clip->position() + clip->clipLength();
                    const int noteRight = clip->kind() == dspx::Clip::Audio || static_cast<dspx::SingingClip *>(clip)->notes()->size() == 0
                                              ? 0
                                              : clip->start() + std::ranges::max(std::views::transform(static_cast<dspx::SingingClip *>(clip)->notes()->asRange(), [](dspx::Note *note) {
                                                    return note->position() + note->length();
                                                }));
                    return std::max({
                        clipRight,
                        noteRight,
                        // TODO param
                    });
                }));
            }))
        }));
    }

    void ProjectWindowInterface::sendOperationConflictNotification() {
        Q_D(ProjectWindowInterface);
        if (!d->operationConflictMessage) {
            d->operationConflictMessage = new Notifications::NotificationMessage(this);
            d->operationConflictMessage->setIcon(SVS::SVSCraft::Warning);
            d->operationConflictMessage->setTitle(tr("Operation Conflict"));
            d->operationConflictMessage->setText(tr("Please wait until the current operation is complete before proceeding."));
        }
        sendNotification(d->operationConflictMessage, AutoHide);
    }

    QWindow *ProjectWindowInterface::createWindow(QObject *parent) const {
        Q_D(const ProjectWindowInterface);
        QQmlComponent component(RuntimeInterface::qmlEngine(), "DiffScope.Core", "ProjectWindow");
        if (component.isError()) {
            qFatal() << component.errorString();
        }
        auto object = component.createWithInitialProperties({{"windowHandle", QVariant::fromValue(this)}
        });
        if (!object) {
            qFatal() << component.errorString();
        }
        auto win = qobject_cast<QQuickWindow *>(object);
        if (!win) {
            qFatal("ProjectWindow in DiffScope.Core is not QQuickWindow");
        }
        SVS::StatusTextContext::setStatusContext(win, new SVS::StatusTextContext(win));
        SVS::StatusTextContext::setContextHelpContext(win, new SVS::StatusTextContext(win));
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
            path = d->projectDocumentContext->defaultDocumentName();
        }
        win->setFilePath(path);
        return win;
    }

    ProjectWindowInterface::ProjectWindowInterface(DspxProject::ProjectDocumentContext *projectDocumentContext, QObject *parent) : ProjectWindowInterface(*new ProjectWindowInterfacePrivate, parent) {
        Q_D(ProjectWindowInterface);
        d->projectDocumentContext = projectDocumentContext;
        if (d->projectDocumentContext->fileLocker()) {
            connect(d->projectDocumentContext->fileLocker(), &FileLocker::pathChanged, this, [=, this] {
                auto win = window();
                if (!win)
                    return;
                auto path = d->projectDocumentContext->fileLocker()->path();
                win->setFilePath(path.isEmpty() ? d->projectDocumentContext->defaultDocumentName() : path);
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
