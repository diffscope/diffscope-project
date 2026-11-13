// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "AudioClipAddOn.h"

#include <memory>

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

#include <CoreApi/runtimeinterface.h>

#include <QAKQuick/quickactioncontext.h>

#include <SVSCraftCore/MusicTimeline.h>
#include <SVSCraftQuick/MessageBox.h>

#include <TalcsFormat/AbstractAudioFormatIO.h>
#include <TalcsWidgets/AudioFileDialog.h>

#include <dspxmodelORM/AudioClip.h>
#include <dspxmodelORM/Clip.h>
#include <dspxmodelORM/ClipSequence.h>
#include <dspxmodelORM/Model.h>
#include <dspxmodelORM/Track.h>
#include <dspxmodelORM/TrackList.h>
#include <dspxmodelSelectionModel/SelectionModel.h>
#include <dspxmodelSelectionModel/TrackSelectionModel.h>

#include <uishell/DialogPositioner.h>

#include <dspxproject/Document.h>
#include <dspxproject/ProjectDocumentContext.h>
#include <dspxproject/ProjectTimeline.h>
#include <coreplugin/ProjectWindowInterface.h>

#include <transactional/TransactionController.h>

#include <audio/AudioClipImporter.h>
#include <audio/GlobalAudioContext.h>
#include <audio/PreviewSoundPlayer.h>
#include <audio/internal/HashHelper.h>
#include <audio/internal/ProjectAudioAddOn.h>

namespace Audio::Internal {

    Q_STATIC_LOGGING_CATEGORY(lcAudioClipAddOn, "diffscope.audio.audioclipaddon")

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
        UIShell::DialogPositioner::positionPopup(dialog, window);
        return dialog;
    }

    static dspx::Track *currentTrack(DspxProject::Document *document) {
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

    AudioClipAddOn::AudioClipAddOn(QObject *parent) : WindowInterfaceAddOn(parent) {
    }

    AudioClipAddOn::~AudioClipAddOn() = default;

    void AudioClipAddOn::initialize() {
        auto windowInterface = windowHandle()->cast<Core::ProjectWindowInterface>();
        windowInterface->addObject(this);

        QQmlComponent component(Core::RuntimeInterface::qmlEngine(), "DiffScope.Audio", "AudioClipAddOnActions");
        if (component.isError()) {
            qFatal() << component.errorString();
        }
        auto o = component.createWithInitialProperties({
            {"addOn", QVariant::fromValue(this)},
        });
        if (!o) {
            qFatal() << component.errorString();
        }
        o->setParent(this);
        QMetaObject::invokeMethod(o, "registerToContext", windowInterface->actionContext());
    }

    void AudioClipAddOn::extensionsInitialized() {
    }

    bool AudioClipAddOn::delayedInitialize() {
        return WindowInterfaceAddOn::delayedInitialize();
    }

    AudioClipAddOn *AudioClipAddOn::of(Core::ProjectWindowInterface *windowHandle) {
        return windowHandle->getFirstObject<AudioClipAddOn>();
    }

    talcs::AbstractAudioFormatIO *AudioClipAddOn::openAudioFile(QString *fileName, QVariant *userData, QString *entryClassName) {
        auto windowInterface = windowHandle()->cast<Core::ProjectWindowInterface>();
        if (!windowInterface)
            return nullptr;

        qCInfo(lcAudioClipAddOn) << "Opening audio file";
        auto state = Core::RuntimeInterface::state();
        state->beginGroup(QStringLiteral("org.diffscope.audio.audioClip"));
        const auto defaultDir = state->value(QStringLiteral("defaultDir"), QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)).toString();
        state->endGroup();
        auto io = talcs::AudioFileDialog::getOpenAudioFileIO(
            GlobalAudioContext::formatManager(),
            *fileName,
            *userData,
            *entryClassName,
            windowInterface->invisibleCentralWidget(),
            tr("Open Audio File"),
            defaultDir
        );
        qCDebug(lcAudioClipAddOn) << "Audio file dialog returned" << io << *fileName << *userData << *entryClassName;
        if (!io) {
            if (fileName->isEmpty()) {
                return nullptr;
            }
            qCWarning(lcAudioClipAddOn) << "Failed to open audio file" << *fileName << *userData << *entryClassName;
            SVS::MessageBox::critical(
                Core::RuntimeInterface::qmlEngine(),
                windowInterface->window(),
                tr("Failed to open audio file"),
                tr("Unable to open \"%1\" as an audio file.").arg(QDir::toNativeSeparators(*fileName))
            );
            return nullptr;
        }

        state->beginGroup(QStringLiteral("org.diffscope.audio.audioClip"));
        state->setValue(QStringLiteral("defaultDir"), QFileInfo(*fileName).absolutePath());
        state->endGroup();
        return io;
    }

    void AudioClipAddOn::insertAudioClip() {
        auto windowInterface = windowHandle()->cast<Core::ProjectWindowInterface>();
        if (!windowInterface || !windowInterface->window() || !windowInterface->projectTimeline())
            return;

        auto document = windowInterface->projectDocumentContext()->document();
        if (!document)
            return;

        Core::TransactionController::ScopedReadLock readLock(document->transactionController());
        if (!readLock) {
            qCWarning(lcAudioClipAddOn) << "Could not acquire a document read lock.";
            windowInterface->sendOperationConflictNotification();
            return;
        }

        auto model = document->model();
        auto trackList = model->tracks();
        if (!trackList || trackList->size() == 0)
            return;

        QString fileName;
        QVariant userData;
        QString entryClassName;
        std::unique_ptr<talcs::AbstractAudioFormatIO> io(openAudioFile(&fileName, &userData, &entryClassName));
        if (!io || !readLock) {
            return;
        }

        AudioClipImporter importer(windowInterface);
        if (!importer.setFile(fileName, io.release(), userData, entryClassName)) {
            return;
        }
        importer.audioFormatIO()->close();

        const QFileInfo fileInfo(fileName);
        auto selectedTrack = currentTrack(document);
        if (!selectedTrack) {
            selectedTrack = trackList->items().first();
        }

        QQmlComponent component(Core::RuntimeInterface::qmlEngine(), "DiffScope.Audio", "InsertAudioClipDialog");
        if (component.isError()) {
            qFatal() << component.errorString();
        }
        QScopedPointer<PreviewSoundPlayer> player(new PreviewSoundPlayer(importer.audioFormatIO(), false, nullptr));
        QVariantMap properties;
        properties.insert("trackList", QVariant::fromValue(trackList));
        properties.insert("selectedTrack", QVariant::fromValue(selectedTrack));
        properties.insert("timeline", QVariant::fromValue(windowInterface->projectTimeline()->musicTimeline()));
        properties.insert("clipPosition", windowInterface->projectTimeline()->position());
        properties.insert("clipName", fileInfo.completeBaseName());
        properties.insert("player", QVariant::fromValue(player.get()));
        auto quickWindow = qobject_cast<QQuickWindow *>(windowInterface->window());
        if (!quickWindow) {
            return;
        }
        auto dialog = createAndPositionDialog(quickWindow, &component, properties);
        if (!execDialog(dialog) || !readLock) {
            return;
        }

        selectedTrack = qobject_cast<dspx::Track *>(dialog->property("selectedTrack").value<QObject *>());
        if (!selectedTrack) {
            selectedTrack = trackList->items().first();
        }
        const auto clipPosition = qMax(0, dialog->property("clipPosition").toInt());
        const auto clipName = dialog->property("clipName").toString();

        player.reset();

        dspx::AudioClip *newClip = nullptr;
        readLock.release();
        document->transactionController()->beginScopedTransaction(tr("Inserting audio clip"), [=, &importer, &newClip] {
            newClip = importer.insertClip(selectedTrack, clipPosition, clipName);
            return newClip != nullptr;
        }, [windowInterface] {
            qCWarning(lcAudioClipAddOn) << "Failed to insert audio clip because the transaction could not start.";
            windowInterface->sendOperationConflictNotification();
        });

        if (newClip) {
            document->selectionModel()->select(newClip, dspx::SelectionModel::Select | dspx::SelectionModel::SetCurrentItem | dspx::SelectionModel::ClearPreviousSelection);
        }
    }

    bool AudioClipAddOn::updateAudioClipToIdenticallyMovedPath(dspx::AudioClip *clip, const QString &filePath) {
        auto windowInterface = windowHandle()->cast<Core::ProjectWindowInterface>();
        if (!windowInterface || !clip || filePath.isEmpty())
            return false;

        auto documentContext = windowInterface->projectDocumentContext();
        auto document = documentContext ? documentContext->document() : nullptr;
        if (!document)
            return false;

        auto path = clip->path();
        AudioClipImporter::updateAudioPathLocation(windowInterface, &path, filePath);
        if (path == clip->path())
            return false;
        return document->transactionController()->beginScopedTransaction(tr("Updating audio clip path"), [=] {
            clip->setPath(path);
            return true;
        }, [windowInterface] {
            qCWarning(lcAudioClipAddOn) << "Failed to update audio clip path because the transaction could not start.";
            windowInterface->sendOperationConflictNotification();
        });
    }

    bool AudioClipAddOn::updateAudioClipDigest(dspx::AudioClip *clip) {
        auto windowInterface = windowHandle()->cast<Core::ProjectWindowInterface>();
        if (!windowInterface || !clip)
            return false;

        auto documentContext = windowInterface->projectDocumentContext();
        auto document = documentContext ? documentContext->document() : nullptr;
        if (!document)
            return false;

        auto path = clip->path();
        const auto filePath = QDir(path.absoluteDir).filePath(path.fileName);
        const auto sha512 = HashHelper::digest(filePath);
        if (sha512.isEmpty()) {
            qCWarning(lcAudioClipAddOn) << "Failed to update audio clip digest from absolute path" << clip << filePath;
            return false;
        }
        if (path.digest.compare(sha512, Qt::CaseInsensitive) == 0) {
            ProjectAudioAddOn::of(windowInterface)->reloadAudioClip(clip);
            return true;
        }

        path.digest = sha512;
        return document->transactionController()->beginScopedTransaction(tr("Updating audio clip digest"), [=] {
            clip->setPath(path);
            return true;
        }, [windowInterface] {
            qCWarning(lcAudioClipAddOn) << "Failed to update audio clip digest because the transaction could not start.";
            windowInterface->sendOperationConflictNotification();
        });
    }

    void AudioClipAddOn::replaceAudioClip(dspx::AudioClip *clip) {
        auto windowInterface = windowHandle()->cast<Core::ProjectWindowInterface>();
        if (!windowInterface || !clip)
            return;

        auto documentContext = windowInterface->projectDocumentContext();
        auto document = documentContext ? documentContext->document() : nullptr;
        if (!document)
            return;

        Core::TransactionController::ScopedReadLock readLock(document->transactionController());
        if (!readLock) {
            qCWarning(lcAudioClipAddOn) << "Could not acquire a document read lock.";
            windowInterface->sendOperationConflictNotification();
            return;
        }

        QString fileName;
        QVariant userData;
        QString entryClassName;
        std::unique_ptr<talcs::AbstractAudioFormatIO> io(openAudioFile(&fileName, &userData, &entryClassName));
        if (!io || !readLock) {
            return;
        }

        const auto path = AudioClipImporter::createAudioPath(windowInterface, fileName, userData, entryClassName);
        auto projectAudioAddOn = ProjectAudioAddOn::of(windowInterface);
        if (path == clip->path()) {
            if (projectAudioAddOn)
                projectAudioAddOn->reloadAudioClip(clip);
            return;
        }
        if (!io->open(talcs::AbstractAudioFormatIO::Read)) {
            return;
        }
        readLock.release();
        document->transactionController()->beginScopedTransaction(tr("Replacing audio clip"), [=, &io] {
            if (projectAudioAddOn) {
                projectAudioAddOn->addAudioClipCache(clip, io.release());
            }
            clip->setPath(path);
            return true;
        }, [windowInterface] {
            qCWarning(lcAudioClipAddOn) << "Failed to replace audio clip because the transaction could not start.";
            windowInterface->sendOperationConflictNotification();
        });
    }

}

#include "moc_AudioClipAddOn.cpp"
