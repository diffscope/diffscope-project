// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "AudioClipAddOn.h"

#include <exception>
#include <memory>
#include <utility>

#include <QDir>
#include <QEventLoop>
#include <QFileDialog>
#include <QFileInfo>
#include <QFutureWatcher>
#include <QLoggingCategory>
#include <QPointer>
#include <QQmlComponent>
#include <QQuickItem>
#include <QQuickWindow>
#include <QRegularExpression>
#include <QScopedPointer>
#include <QSettings>
#include <QStandardPaths>
#include <QVariant>

#include <CoreApi/filelocker.h>
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
#include <audio/AudioClipAudioContext.h>
#include <audio/EmbeddedAudio.h>
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
        QObject::connect(dialog, SIGNAL(closed()), &eventLoop, SLOT(quit()));
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

    AudioClipAddOn::~AudioClipAddOn() {
        if (m_embeddingWatcher) {
            m_embeddingWatcher->cancel();
        }
    }

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
        auto *window = windowHandle()->cast<Core::ProjectWindowInterface>();
        EmbeddedAudio::of(window->projectDocumentContext())->configureClipboard(window->clipboardEditScenario(), [window = QPointer(window)] {
            if (window) {
                SVS::MessageBox::warning(Core::RuntimeInterface::qmlEngine(), window->window(), tr("Copy Failed"),
                                         tr("The selection contains unavailable embedded audio."));
            }
        });
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

    void AudioClipAddOn::embedAudioClip(dspx::AudioClip *clip) {
        const auto window = QPointer(windowHandle()->cast<Core::ProjectWindowInterface>());
        auto *audioContext = clip ? AudioClipAudioContext::of(clip) : nullptr;
        if (m_embeddingWatcher || !window || !audioContext || !clip->path().embeddedKey.isEmpty()) {
            return;
        }
        auto *quickWindow = qobject_cast<QQuickWindow *>(window->window());
        const auto context = QPointer(window->projectDocumentContext());
        auto *document = context ? context->document() : nullptr;
        if (!quickWindow || !document) {
            return;
        }
        auto readLock = std::make_unique<Core::TransactionController::ScopedReadLock>(document->transactionController());
        if (!*readLock) {
            window->sendOperationConflictNotification();
            return;
        }

        const auto guard = QPointer(this);
        const auto target = QPointer(clip);
        const auto path = clip->path();
        const auto sourceFilePath = audioContext->realAudioPath();
        auto *engine = Core::RuntimeInterface::qmlEngine();
        QQmlComponent optionsComponent(engine, "DiffScope.Audio", "EmbedAudioDialog");
        if (optionsComponent.isError()) {
            qFatal() << optionsComponent.errorString();
        }
        QScopedPointer<QObject, QScopedPointerDeleteLater> options(createAndPositionDialog(quickWindow, &optionsComponent, {}));
        if (!execDialog(options.get()) || !guard || !*readLock || m_embeddingWatcher) {
            return;
        }
        const auto compression = options->property("lossy").toBool() ? EmbeddedAudio::Compression::Lossy : EmbeddedAudio::Compression::Lossless;
        auto *progress = createAndPositionDialog(quickWindow, &optionsComponent, {{QStringLiteral("preparing"), true}});
        progress->setParent(this);
        auto *watcher = new QFutureWatcher<EmbeddedAudio::Prepared>(this);
        m_embeddingWatcher = watcher;
        connect(progress, SIGNAL(rejected()), watcher, SLOT(cancel()));
        connect(progress, SIGNAL(closed()), watcher, SLOT(cancel()));
        connect(watcher, &QFutureWatcherBase::finished, this, [this, guard, watcher, progress = QPointer(progress), window, context, target, readLock = std::move(readLock)]() mutable {
            const auto lock = std::move(readLock);
            m_embeddingWatcher = nullptr;
            watcher->deleteLater();
            const bool cancelled = watcher->isCanceled();
            if (progress) {
                QObject::disconnect(progress, nullptr, watcher, nullptr);
                QMetaObject::invokeMethod(progress, "close");
                progress->deleteLater();
            }
            if (cancelled || !*lock || !window || !context || !target) {
                return;
            }
            const auto report = [window](const QString &message) {
                if (window) {
                    SVS::MessageBox::warning(Core::RuntimeInterface::qmlEngine(), window->window(), tr("Embed Audio Failed"), message);
                }
            };
            EmbeddedAudio::Prepared prepared;
            try {
                prepared = watcher->result();
            } catch (const std::exception &error) {
                qCWarning(lcAudioClipAddOn) << "Failed to prepare embedded audio" << error.what();
                report(tr("Could not store the embedded audio."));
                return;
            }
            if (!prepared.error().isEmpty()) {
                report(prepared.error());
                return;
            }
            auto *store = EmbeddedAudio::of(context);
            const auto conflict = store->conflict(prepared);
            const bool downgrade = conflict.action == EmbeddedAudio::Conflict::Downgrade;
            if (downgrade) {
                const auto message = conflict.referenceCount > 0
                    ? tr("Other clips using this audio will also use lossy encoding. The number of affected clips is %L1.").arg(conflict.referenceCount)
                    : tr("The lossless copy will be replaced. Any clips restored by undo will use the lossy copy.");
                if (SVS::MessageBox::warning(Core::RuntimeInterface::qmlEngine(), window->window(), tr("Replace Lossless Audio"), message,
                                             SVS::SVSCraft::Yes | SVS::SVSCraft::Cancel, SVS::SVSCraft::Cancel) != SVS::SVSCraft::Yes
                    || !guard || !*lock) {
                    return;
                }
            }
            lock->release();
            const auto result = store->apply({prepared}, {conflict}, tr("Embedding audio clip"), [target](const QList<dspx::AudioPathInfo> &paths) {
                target->setPath(paths.first());
                return true;
            }, downgrade);
            if ((result == EmbeddedAudio::ApplyResult::Busy || result == EmbeddedAudio::ApplyResult::Stale) && window) {
                window->sendOperationConflictNotification();
            } else if (result == EmbeddedAudio::ApplyResult::WriteFailed) {
                report(tr("Could not store the embedded audio."));
            }
        });
        QMetaObject::invokeMethod(progress, "open");
        watcher->setFuture(EmbeddedAudio::prepare(sourceFilePath, path.userData, path.formatEntryClassName, compression));
    }

    void AudioClipAddOn::relinkAudioClip(dspx::AudioClip *clip) {
        const auto window = QPointer(windowHandle()->cast<Core::ProjectWindowInterface>());
        const auto target = QPointer(clip);
        auto *audioContext = target ? AudioClipAudioContext::of(target) : nullptr;
        if (!window || !window->window() || !audioContext || audioContext->status() != AudioClipAudioContext::Ready) {
            return;
        }
        const auto key = target->path().embeddedKey;
        auto *context = window->projectDocumentContext();
        auto *document = context ? context->document() : nullptr;
        if (!document || key.isEmpty()) {
            return;
        }
        Core::TransactionController::ScopedReadLock readLock(document->transactionController());
        if (!readLock) {
            window->sendOperationConflictNotification();
            return;
        }

        const auto *locker = context->fileLocker();
        const auto projectPath = locker ? locker->path() : QString();
        const QDir directory(projectPath.isEmpty()
            ? QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)
            : QFileInfo(projectPath).absolutePath());
        auto baseName = target->name().trimmed();
        baseName.replace(QRegularExpression(QStringLiteral("[<>:\"/\\\\|?*\\x00-\\x1f]")), QStringLiteral("_"));
        baseName.remove(QRegularExpression(QStringLiteral("[ .]+$")));
        if (baseName.isEmpty()) {
            baseName = tr("Audio Clip");
        }
        static const QRegularExpression reservedName(
            QStringLiteral("^(CON|PRN|AUX|NUL|COM[1-9]|LPT[1-9])(?:\\..*)?$"),
            QRegularExpression::CaseInsensitiveOption
        );
        if (reservedName.match(baseName).hasMatch()) {
            baseName.prepend(u'_');
        }
        const auto suffix = audioContext->embeddedLossless() ? QStringLiteral("wav") : QStringLiteral("ogg");
        QFileDialog dialog;
        dialog.setWindowTitle(tr("Relink to File"));
        dialog.setAcceptMode(QFileDialog::AcceptSave);
        dialog.setNameFilter(tr("Audio File (*.%1)").arg(suffix));
        dialog.setDefaultSuffix(suffix);
        dialog.setDirectory(directory);
        dialog.selectFile(baseName + u'.' + suffix);
        dialog.winId();
        dialog.windowHandle()->setTransientParent(window->window());
        dialog.setWindowModality(Qt::WindowModal);
        const auto filePath = dialog.exec() == QDialog::Accepted ? dialog.selectedFiles().value(0) : QString();
        if (filePath.isEmpty() || !readLock || !window || !target || target->path().embeddedKey != key) {
            return;
        }
        if (QFileInfo(filePath).suffix().compare(suffix, Qt::CaseInsensitive) != 0) {
            SVS::MessageBox::warning(Core::RuntimeInterface::qmlEngine(), window->window(), tr("Relink Failed"),
                                     tr("Choose a file with the .%1 extension.").arg(suffix));
            return;
        }
        if (!EmbeddedAudio::of(context)->extractToFile(key, filePath)) {
            SVS::MessageBox::warning(Core::RuntimeInterface::qmlEngine(), window->window(), tr("Relink Failed"),
                                     tr("Could not extract the embedded audio."));
            return;
        }

        const auto path = AudioClipImporter::createAudioPath(window, filePath, {}, {});
        readLock.release();
        document->transactionController()->beginScopedTransaction(tr("Relinking audio clip"), [target, path] {
            target->setPath(path);
            return true;
        }, [window] {
            window->sendOperationConflictNotification();
        });
    }

}

#include "moc_AudioClipAddOn.cpp"
