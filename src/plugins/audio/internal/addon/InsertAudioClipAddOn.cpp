#include "InsertAudioClipAddOn.h"

#include <QDir>
#include <QEventLoop>
#include <QFileInfo>
#include <QLoggingCategory>
#include <QQmlComponent>
#include <QQuickItem>
#include <QQuickWindow>
#include <QSettings>
#include <QStandardPaths>
#include <QVariant>

#include <CoreApi/filelocker.h>
#include <CoreApi/runtimeinterface.h>

#include <QAKQuick/quickactioncontext.h>

#include <SVSCraftCore/MusicTimeline.h>
#include <SVSCraftCore/MusicTime.h>
#include <SVSCraftQuick/MessageBox.h>

#include <TalcsFormat/AbstractAudioFormatIO.h>
#include <TalcsWidgets/AudioFileDialog.h>

#include <dspxmodel/AudioClip.h>
#include <dspxmodel/BusControl.h>
#include <dspxmodel/Clip.h>
#include <dspxmodel/ClipSequence.h>
#include <dspxmodel/ClipTime.h>
#include <dspxmodel/Model.h>
#include <dspxmodel/SelectionModel.h>
#include <dspxmodel/Track.h>
#include <dspxmodel/TrackList.h>
#include <dspxmodel/TrackSelectionModel.h>

#include <coreplugin/DspxDocument.h>
#include <coreplugin/ProjectDocumentContext.h>
#include <coreplugin/ProjectTimeline.h>
#include <coreplugin/ProjectWindowInterface.h>

#include <transactional/TransactionController.h>

#include <audio/GlobalAudioContext.h>
#include <audio/internal/HashHelper.h>
#include <audio/internal/ProjectAudioAddOn.h>

namespace Audio::Internal {

    Q_STATIC_LOGGING_CATEGORY(lcInsertAudioClipAddOn, "diffscope.audio.insertaudioclipaddon")

    static bool execDialog(QObject *dialog) {
        QEventLoop eventLoop;
        QObject::connect(dialog, SIGNAL(accepted()), &eventLoop, SLOT(quit()));
        QObject::connect(dialog, SIGNAL(rejected()), &eventLoop, SLOT(quit()));
        QMetaObject::invokeMethod(dialog, "open");
        eventLoop.exec();
        return dialog->property("result").toInt() == 1;
    }

    static QObject *createAndPositionDialog(QQuickWindow *window, QQmlComponent *component, const QVariantMap &initialProperties) {
        if (component->isError()) {
            qFatal() << component->errorString();
        }
        QVariantMap properties = initialProperties;
        properties.insert("parent", QVariant::fromValue(window->contentItem()));
        auto dialog = component->createWithInitialProperties(properties);
        if (!dialog) {
            qFatal() << component->errorString();
        }
        auto width = dialog->property("width").toDouble();
        auto height = dialog->property("height").toDouble();
        dialog->setProperty("x", window->width() / 2.0 - width / 2);
        if (auto popupTopMarginHint = window->property("popupTopMarginHint"); popupTopMarginHint.isValid()) {
            dialog->setProperty("y", popupTopMarginHint);
        } else {
            dialog->setProperty("y", window->height() / 2.0 - height / 2);
        }
        return dialog;
    }

    static dspx::Track *currentTrack(Core::DspxDocument *document) {
        auto selectionModel = document ? document->selectionModel() : nullptr;
        auto trackSelectionModel = selectionModel ? selectionModel->trackSelectionModel() : nullptr;
        auto track = trackSelectionModel ? trackSelectionModel->currentItem() : nullptr;
        if (!track) {
            if (auto currentClip = qobject_cast<dspx::Clip *>(selectionModel ? selectionModel->currentItem() : nullptr)) {
                if (auto clipSequence = currentClip->clipSequence()) {
                    track = clipSequence->track();
                }
            }
        }
        return track;
    }

    static bool isInDirectoryOrSubdirectory(const QString &filePath, const QDir &directory) {
        const auto relativePath = directory.relativeFilePath(QFileInfo(filePath).absoluteFilePath());
        return relativePath == "." || (!relativePath.startsWith("..") && !QDir::isAbsolutePath(relativePath));
    }

    InsertAudioClipAddOn::InsertAudioClipAddOn(QObject *parent) : WindowInterfaceAddOn(parent) {
    }

    InsertAudioClipAddOn::~InsertAudioClipAddOn() = default;

    void InsertAudioClipAddOn::initialize() {
        auto windowInterface = windowHandle()->cast<Core::ProjectWindowInterface>();
        windowInterface->addObject(this);

        QQmlComponent component(Core::RuntimeInterface::qmlEngine(), "DiffScope.Audio", "InsertAudioClipAddOnActions");
        if (component.isError()) {
            qFatal() << component.errorString();
        }
        auto o = component.createWithInitialProperties({
            {"addOn", QVariant::fromValue(this)},
        });
        o->setParent(this);
        QMetaObject::invokeMethod(o, "registerToContext", windowInterface->actionContext());
    }

    void InsertAudioClipAddOn::extensionsInitialized() {
    }

    bool InsertAudioClipAddOn::delayedInitialize() {
        return WindowInterfaceAddOn::delayedInitialize();
    }

    InsertAudioClipAddOn *InsertAudioClipAddOn::of(Core::ProjectWindowInterface *windowHandle) {
        return windowHandle->getFirstObject<InsertAudioClipAddOn>();
    }

    void InsertAudioClipAddOn::insertAudioClip() {
        auto windowInterface = windowHandle()->cast<Core::ProjectWindowInterface>();
        if (!windowInterface || !windowInterface->window() || !windowInterface->projectTimeline())
            return;

        auto document = windowInterface->projectDocumentContext()->document();
        if (!document)
            return;

        auto model = document->model();
        auto trackList = model->tracks();
        if (!trackList || trackList->size() == 0)
            return;

        qCInfo(lcInsertAudioClipAddOn) << "Opening audio file for insertion";
        QString fileName;
        QVariant userData;
        QString entryClassName;
        auto settings = Core::RuntimeInterface::settings();
        settings->beginGroup(staticMetaObject.className());
        const auto defaultDir = settings->value(QStringLiteral("defaultDir"), QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)).toString();
        settings->endGroup();
        auto io = talcs::AudioFileDialog::getOpenAudioFileIO(
            GlobalAudioContext::formatManager(),
            fileName,
            userData,
            entryClassName,
            windowInterface->invisibleCentralWidget(),
            tr("Open Audio File"),
            defaultDir
        );
        qCDebug(lcInsertAudioClipAddOn) << "Audio file dialog returned" << io << fileName << userData << entryClassName;
        if (!io) {
            if (fileName.isEmpty()) {
                return;
            }
            qCWarning(lcInsertAudioClipAddOn) << "Failed to open audio file" << fileName << userData << entryClassName;
            SVS::MessageBox::critical(
                Core::RuntimeInterface::qmlEngine(),
                windowInterface->window(),
                tr("Failed to open audio file"),
                tr("Unable to open \"%1\" as an audio file.").arg(QDir::toNativeSeparators(fileName))
            );
            return;
        }

        const QFileInfo fileInfo(fileName);
        settings->beginGroup(staticMetaObject.className());
        settings->setValue(QStringLiteral("defaultDir"), fileInfo.absolutePath());
        settings->endGroup();

        auto selectedTrack = currentTrack(document);
        if (!selectedTrack) {
            selectedTrack = trackList->items().first();
        }

        QQmlComponent component(Core::RuntimeInterface::qmlEngine(), "DiffScope.Audio", "InsertAudioClipDialog");
        QVariantMap properties;
        properties.insert("trackList", QVariant::fromValue(trackList));
        properties.insert("selectedTrack", QVariant::fromValue(selectedTrack));
        properties.insert("timeline", QVariant::fromValue(windowInterface->projectTimeline()->musicTimeline()));
        properties.insert("clipPosition", windowInterface->projectTimeline()->position());
        properties.insert("clipName", fileInfo.completeBaseName());
        auto quickWindow = qobject_cast<QQuickWindow *>(windowInterface->window());
        if (!quickWindow) {
            delete io;
            return;
        }
        auto dialog = createAndPositionDialog(quickWindow, &component, properties);
        if (!execDialog(dialog)) {
            delete io;
            return;
        }

        selectedTrack = qobject_cast<dspx::Track *>(dialog->property("selectedTrack").value<QObject *>());
        if (!selectedTrack) {
            selectedTrack = trackList->items().first();
        }
        const auto clipPosition = qMax(0, dialog->property("clipPosition").toInt());
        const auto clipName = dialog->property("clipName").toString();

        auto timeline = windowInterface->projectTimeline()->musicTimeline();
        io->open(talcs::AbstractAudioFormatIO::Read);
        const auto durationMsec = io->sampleRate() > 0 ? static_cast<double>(io->length()) * 1000.0 / io->sampleRate() : 0.0;
        const auto clipPositionMsec = timeline->create(0, 0, clipPosition).millisecond();
        const auto clipEndPosition = timeline->create(clipPositionMsec + durationMsec).totalTick();
        const auto clipLength = qMax(1, clipEndPosition - clipPosition);

        dspx::AudioPathInfo path;
        path.absoluteDir = fileInfo.absolutePath();
        path.fileName = fileInfo.fileName();
        path.formatEntryClassName = entryClassName;
        path.userData = userData;
        path.sha512 = HashHelper::sha512(fileInfo.absoluteFilePath());

        auto fileLocker = windowInterface->projectDocumentContext()->fileLocker();
        const auto projectPath = fileLocker ? fileLocker->path() : QString();
        if (!projectPath.isEmpty()) {
            const QFileInfo projectFileInfo(projectPath);
            const QDir projectDir(projectFileInfo.absolutePath());
            if (isInDirectoryOrSubdirectory(fileInfo.absoluteFilePath(), projectDir)) {
                path.relativeDir = projectDir.relativeFilePath(fileInfo.absolutePath());
            }
        }

        qCDebug(lcInsertAudioClipAddOn) << "Inserting audio clip" << fileName << "at" << clipPosition
                                        << "length" << clipLength << "track" << selectedTrack;

        auto projectAudioAddOn = ProjectAudioAddOn::of(windowInterface);
        dspx::AudioClip *newClip = nullptr;
        bool success = false;
        bool cacheTransferred = false;
        document->transactionController()->beginScopedTransaction(tr("Inserting audio clip"), [=, &newClip, &success, &cacheTransferred] {
            newClip = model->createAudioClip();
            newClip->setName(clipName);
            newClip->setPath(path);
            auto time = newClip->time();
            time->setClipStart(0);
            time->setClipLen(clipLength);
            time->setStart(clipPosition);
            newClip->control()->setGain(1);
            if (projectAudioAddOn) {
                projectAudioAddOn->addAudioClipCache(newClip, io);
                cacheTransferred = true;
            }
            auto clipSequence = selectedTrack->clips();
            if (!clipSequence || !clipSequence->insertItem(newClip)) {
                if (projectAudioAddOn && cacheTransferred) {
                    delete projectAudioAddOn->takeAudioClipCache(newClip);
                    cacheTransferred = false;
                }
                model->destroyItem(newClip);
                newClip = nullptr;
                return false;
            }
            success = true;
            return true;
        }, [] {
            qCCritical(lcInsertAudioClipAddOn) << "Failed to insert audio clip in exclusive transaction";
        });

        if (!success) {
            if (!cacheTransferred) {
                delete io;
            }
            return;
        }
        if (!cacheTransferred) {
            delete io;
        }

        if (newClip) {
            document->selectionModel()->select(newClip, dspx::SelectionModel::Select | dspx::SelectionModel::SetCurrentItem | dspx::SelectionModel::ClearPreviousSelection);
        }
    }

}

#include "moc_InsertAudioClipAddOn.cpp"
