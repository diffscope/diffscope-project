// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "AudioExtractionAddOn.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <memory>
#include <optional>
#include <utility>

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QList>
#include <QLoggingCategory>
#include <QMap>
#include <QQmlComponent>
#include <QQuickItem>
#include <QQuickWindow>
#include <QRegularExpression>
#include <QSaveFile>
#include <QSet>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QVariant>

#include <CoreApi/applicationinfo.h>
#include <CoreApi/filelocker.h>
#include <CoreApi/runtimeinterface.h>

#include <opendspx/track.h>

#include <QAKQuick/quickactioncontext.h>

#include <SVSCraftCore/MusicTime.h>
#include <SVSCraftCore/MusicTimeSignature.h>
#include <SVSCraftCore/MusicTimeline.h>
#include <SVSCraftCore/SVSCraftNamespace.h>
#include <SVSCraftQuick/MessageBox.h>

#include <dspxmodelORM/ClipSequence.h>
#include <dspxmodelORM/FreeValueDataArray.h>
#include <dspxmodelORM/Model.h>
#include <dspxmodelORM/Note.h>
#include <dspxmodelORM/NoteSequence.h>
#include <dspxmodelORM/Parameter.h>
#include <dspxmodelORM/ParameterMap.h>
#include <dspxmodelORM/SingingClip.h>
#include <dspxmodelORM/Tempo.h>
#include <dspxmodelORM/TempoSequence.h>
#include <dspxmodelORM/Track.h>
#include <dspxmodelORM/TrackList.h>
#include <dspxmodelSelectionModel/SelectionModel.h>

#include <uishell/DialogPositioner.h>

#include <coreplugin/CoreInterface.h>
#include <dspxproject/Document.h>
#include <dspxproject/ProjectDocumentContext.h>
#include <dspxproject/ProjectTimeline.h>
#include <coreplugin/ProjectWindowInterface.h>
#include <coreplugin/SingerSettings.h>
#include <singercatalog/ArchitectureInfo.h>
#include <singercatalog/DefaultLyricManager.h>

#include <audio/AudioClipAudioContext.h>
#include <audio/EmbeddedAudio.h>
#include <synth/internal/TempoExtractionPreview.h>
#include <transactional/TransactionController.h>

namespace Synth::Internal {

    Q_STATIC_LOGGING_CATEGORY(lcAudioExtractionScenario, "diffscope.synth.extraction.scenario")

    namespace {

        QString safeName(QString name) {
            name.replace(QRegularExpression(QStringLiteral("[<>:\"/\\\\|?*\\x00-\\x1f]")), QStringLiteral("_"));
            name.remove(QRegularExpression(QStringLiteral("[ .]+$")));
            if (name.isEmpty())
                name = QStringLiteral("audio");
            static const QRegularExpression reserved(QStringLiteral("^(CON|PRN|AUX|NUL|COM[1-9]|LPT[1-9])(?:\\..*)?$"), QRegularExpression::CaseInsensitiveOption);
            if (reserved.match(name).hasMatch())
                name.prepend(u'_');
            return name;
        }

        QString fileKey(const QString &path) {
#ifdef Q_OS_WIN
            return path.toCaseFolded();
#else
            return path;
#endif
        }

        bool copyFile(const QString &source, const QString &target) {
            QFile input(source);
            QSaveFile output(target);
            if (!input.open(QIODevice::ReadOnly) || !output.open(QIODevice::WriteOnly))
                return false;
            while (!input.atEnd()) {
                const auto bytes = input.read(1024 * 1024);
                if (bytes.isEmpty() || output.write(bytes) != bytes.size())
                    return false;
            }
            return output.commit();
        }

        QString temporaryDirectoryPath(const QString &name) {
            const auto base = QDir(Core::ApplicationInfo::applicationLocation(Core::ApplicationInfo::TempData)).filePath(QStringLiteral("synth"));
            QDir().mkpath(base);
            return QDir(base).filePath(name + QStringLiteral("-XXXXXX"));
        }

        QList<QVariant> resamplePitch(const AudioExtractionResult &result, SVS::MusicTimeline *timeline, int position, int length) {
            const int step = dspx::FreeValueDataArray::step();
            const int count = length / step + (length % step != 0);
            const double startMsec = timeline->create(0, 0, position).millisecond();
            QList<QVariant> values;
            values.reserve(count);
            auto segment = result.pitch.cbegin();
            for (int index = 0; index < count; ++index) {
                const double seconds = (timeline->create(0, 0, position + index * step).millisecond() - startMsec) / 1000;
                while (segment != result.pitch.cend() && seconds >= segment->startSeconds + segment->cents.size() / result.pitchSampleRate)
                    ++segment;
                if (segment == result.pitch.cend() || seconds < segment->startSeconds || segment->cents.isEmpty()) {
                    values.append(QVariant());
                    continue;
                }
                const double samplePosition = (seconds - segment->startSeconds) * result.pitchSampleRate;
                const auto left = std::min(static_cast<qsizetype>(std::floor(samplePosition)), segment->cents.size() - 1);
                const auto right = std::min(left + 1, segment->cents.size() - 1);
                double rightCent = segment->cents.at(right);
                const auto next = segment + 1;
                if (left == right && next != result.pitch.cend() && !next->cents.isEmpty()
                    && qFuzzyCompare(next->startSeconds, segment->startSeconds + segment->cents.size() / result.pitchSampleRate)) {
                    rightCent = next->cents.first();
                }
                const double cent = std::lerp(segment->cents.at(left), rightCent, samplePosition - left);
                values.append(SingerCatalog::ParameterInfo::toDspxModelValue(cent / 12800.0));
            }
            return values;
        }

    }

    struct AudioExtractionAddOn::Operation {
        Core::TransactionController::ScopedReadLock readLock;
        QPointer<DspxProject::Document> document;
        QPointer<dspx::AudioClip> source;
        QPointer<dspx::Track> track;
        AudioExtractionTask::Type type{};
        QString name;
        QString directory;
        dspx::AudioPathInfo sourcePath;
        ExtractionAudioCodec::Input input;
        int position{};
        int clipStart{};
        int clipLength{};
        int length{};
        QMap<int, double> tempi;
        QMap<int, SVS::MusicTimeSignature> timeSignatures;
        qint64 fileSize{};
        QDateTime fileModified;
        QMap<QString, QPair<qint64, QDateTime>> overwriteFiles;
    };

    AudioExtractionAddOn::AudioExtractionAddOn(QObject *parent) : WindowInterfaceAddOn(parent) {
    }

    AudioExtractionAddOn::~AudioExtractionAddOn() {
        if (m_task)
            m_task->cancel();
        delete m_pendingDialog.data();
        delete m_dialog.data();
    }

    void AudioExtractionAddOn::initialize() {
        auto windowInterface = windowHandle()->cast<Core::ProjectWindowInterface>();
        windowInterface->addObject(this);
        QQmlComponent component(Core::RuntimeInterface::qmlEngine(), "DiffScope.Synth", "AudioExtractionActions");
        if (component.isError())
            qFatal() << component.errorString();
        auto actions = component.createWithInitialProperties({{QStringLiteral("addOn"), QVariant::fromValue(this)}});
        if (!actions)
            qFatal() << component.errorString();
        actions->setParent(this);
        QMetaObject::invokeMethod(actions, "registerToContext", windowInterface->actionContext());
    }

    void AudioExtractionAddOn::extensionsInitialized() {
    }

    void AudioExtractionAddOn::extractNotes(dspx::AudioClip *clip) { open(clip, AudioExtractionTask::Notes); }
    void AudioExtractionAddOn::extractTempo(dspx::AudioClip *clip) { open(clip, AudioExtractionTask::Tempo); }
    void AudioExtractionAddOn::extractPitch(dspx::AudioClip *clip) { open(clip, AudioExtractionTask::Pitch); }
    void AudioExtractionAddOn::separateAudio(dspx::AudioClip *clip) { open(clip, AudioExtractionTask::Separation); }

    QString AudioExtractionAddOn::defaultOutputDirectory() const {
        const auto *windowInterface = windowHandle()->cast<Core::ProjectWindowInterface>();
        const auto *locker = windowInterface->projectDocumentContext()->fileLocker();
        const auto projectPath = locker ? locker->path() : QString();
        return normalizedPath(projectPath.isEmpty()
            ? QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)
            : QFileInfo(projectPath).absolutePath());
    }

    QString AudioExtractionAddOn::chooseDirectory(const QString &directory) {
        auto windowInterface = windowHandle()->cast<Core::ProjectWindowInterface>();
        const auto startDirectory = normalizedPath(directory);
        return normalizedPath(QFileDialog::getExistingDirectory(windowInterface->invisibleCentralWidget(), tr("Choose Separated Audio Directory"),
            startDirectory.isEmpty() ? defaultOutputDirectory() : startDirectory));
    }

    QString AudioExtractionAddOn::normalizedPath(const QString &path) const {
        if (path.isEmpty())
            return {};
        const QFileInfo info(QDir::fromNativeSeparators(path));
        const auto canonicalPath = info.canonicalFilePath();
        return QDir::toNativeSeparators(QDir::cleanPath(canonicalPath.isEmpty() ? info.absoluteFilePath() : canonicalPath));
    }

    void AudioExtractionAddOn::open(dspx::AudioClip *clip, AudioExtractionTask::Type type) {
        auto windowInterface = windowHandle()->cast<Core::ProjectWindowInterface>();
        auto window = qobject_cast<QQuickWindow *>(windowInterface->window());
        if (m_dialog || m_pendingDialog || !window || !clip || !clip->clipSequence() || clip->clipLength() <= 0)
            return;
        auto operation = std::make_unique<Operation>();
        operation->document = windowInterface->projectDocumentContext()->document();
        if (!operation->document)
            return;
        if (!operation->readLock.acquire(operation->document->transactionController())) {
            qCWarning(lcAudioExtractionScenario) << "Could not acquire a document read lock.";
            windowInterface->sendOperationConflictNotification();
            return;
        }
        const bool separation = type == AudioExtractionTask::Separation;
        auto outputMode = LinkToFile;
        if (separation && !clip->path().embeddedKey.isEmpty()) {
            const auto *audioContext = Audio::AudioClipAudioContext::of(clip);
            outputMode = audioContext && audioContext->embeddedLossless() ? EmbedLossless : EmbedLossy;
        }
        m_operation = std::move(operation);
        m_operation->source = clip;
        m_operation->type = type;
        m_task = new AudioExtractionTask(type, this);
        QQmlComponent component(Core::RuntimeInterface::qmlEngine(), "DiffScope.Synth", "AudioExtractionDialog");
        if (component.isError())
            qFatal() << component.errorString();
        m_dialog = component.createWithInitialProperties({
            {QStringLiteral("parent"), QVariant::fromValue(window->contentItem())},
            {QStringLiteral("addOn"), QVariant::fromValue(this)},
            {QStringLiteral("task"), QVariant::fromValue(m_task.data())},
            {QStringLiteral("separation"), separation},
            {QStringLiteral("directory"), separation ? defaultOutputDirectory() : QString()},
            {QStringLiteral("outputMode"), static_cast<int>(outputMode)},
        });
        if (!m_dialog)
            qFatal() << component.errorString();
        if (type == AudioExtractionTask::Tempo)
            m_dialog->setProperty("title", tr("Extract Tempo"));
        else if (type == AudioExtractionTask::Pitch)
            m_dialog->setProperty("title", tr("Extract Pitch"));
        m_task->setParent(m_dialog);
        UIShell::DialogPositioner::positionPopup(m_dialog, window);
        connect(m_task, &AudioExtractionTask::succeeded, this, &AudioExtractionAddOn::finish);
        connect(m_task, &AudioExtractionTask::changed, this, [this, task = m_task.data()] {
            if (m_operation && m_task == task && !task->busy() && !task->error().isEmpty())
                m_operation->readLock.release();
        });
        QMetaObject::invokeMethod(m_dialog, "open");
        m_task->refresh();
        qCDebug(lcAudioExtractionScenario) << "Entered pending state";
    }

    bool AudioExtractionAddOn::sourceAvailable() const {
        if (!m_operation || !m_operation->source || !m_operation->document)
            return false;
        auto windowInterface = windowHandle()->cast<Core::ProjectWindowInterface>();
        auto clip = m_operation->source.data();
        return windowInterface->projectDocumentContext()->document() == m_operation->document
            && clip->model() == m_operation->document->model() && clip->clipSequence()
            && m_operation->document->model()->tracks()->items().contains(clip->clipSequence()->track());
    }

    void AudioExtractionAddOn::start(int extractorIndex, int outputMode, const QString &directory, int outputTrackCount, const QList<int> &outputRoutes) {
        if (!m_task || m_task->busy() || m_task->refreshing())
            return;
        if (!sourceAvailable()) {
            showError(tr("The original audio clip is no longer available."));
            return;
        }
        m_dialog->setProperty("scenarioError", QString());
        auto &operation = *m_operation;
        if (!operation.readLock.acquire(operation.document->transactionController())) {
            qCWarning(lcAudioExtractionScenario) << "Could not acquire a document read lock.";
            windowHandle()->cast<Core::ProjectWindowInterface>()->sendOperationConflictNotification();
            return;
        }
        auto clip = operation.source.data();
        auto audioContext = Audio::AudioClipAudioContext::of(clip);
        const auto sourceFile = audioContext ? audioContext->realAudioPath() : QString();
        if (sourceFile.isEmpty() || !QFileInfo(sourceFile).isFile()) {
            showError(tr("The audio file for the selected clip is unavailable."));
            return;
        }
        operation.overwriteFiles.clear();
        std::optional<Audio::EmbeddedAudio::Compression> compression;
        if (operation.type == AudioExtractionTask::Separation && outputMode != LinkToFile) {
            compression = outputMode == EmbedLossy ? Audio::EmbeddedAudio::Compression::Lossy : Audio::EmbeddedAudio::Compression::Lossless;
        } else if (operation.type == AudioExtractionTask::Separation) {
            const auto outputDirectory = normalizedPath(directory);
            m_dialog->setProperty("directory", outputDirectory);
            const QFileInfo directoryInfo(outputDirectory);
            if (!directoryInfo.isDir() || !directoryInfo.isWritable()) {
                showError(tr("Choose a writable directory for the separated audio."));
                return;
            }
            operation.directory = directoryInfo.absoluteFilePath();
            const auto prefix = safeName(clip->name()) + u'_';
            QStringList conflicts;
            const auto entries = QDir(operation.directory).entryInfoList(QDir::Files | QDir::Hidden | QDir::System | QDir::NoDotAndDotDot);
            for (const auto &entry : entries) {
                if (entry.fileName().startsWith(prefix, Qt::CaseInsensitive) && entry.suffix().compare(QStringLiteral("wav"), Qt::CaseInsensitive) == 0) {
                    conflicts.append(normalizedPath(entry.absoluteFilePath()));
                    operation.overwriteFiles.insert(fileKey(entry.absoluteFilePath()), {entry.size(), entry.lastModified()});
                }
            }
            if (!conflicts.isEmpty()) {
                auto windowInterface = windowHandle()->cast<Core::ProjectWindowInterface>();
                const auto answer = SVS::MessageBox::warning(Core::RuntimeInterface::qmlEngine(), windowInterface->window(),
                    tr("Overwrite Separated Audio"),
                    tr("The following files may be replaced by the separated tracks. Overwrite matching files?\n%1").arg(conflicts.join(u'\n')),
                    SVS::SVSCraft::Yes | SVS::SVSCraft::Cancel, SVS::SVSCraft::Cancel);
                if (answer != SVS::SVSCraft::Yes)
                    return;
                if (!sourceAvailable()) {
                    showError(tr("The original audio clip is no longer available."));
                    return;
                }
            }
        }
        auto windowInterface = windowHandle()->cast<Core::ProjectWindowInterface>();
        auto timeline = windowInterface->projectTimeline()->musicTimeline();
        operation.track = clip->clipSequence()->track();
        operation.name = clip->name();
        operation.position = clip->position();
        operation.clipStart = clip->clipStart();
        operation.clipLength = clip->clipLength();
        operation.length = clip->length();
        operation.sourcePath = clip->path();
        operation.tempi = timeline->tempi();
        operation.timeSignatures = timeline->timeSignatures();
        operation.fileSize = QFileInfo(sourceFile).size();
        operation.fileModified = QFileInfo(sourceFile).lastModified();
        const double start = timeline->create(0, 0, clip->position()).millisecond();
        const double contentStart = timeline->create(0, 0, clip->start()).millisecond();
        const double end = timeline->create(0, 0, clip->position() + clip->clipLength()).millisecond();
        operation.input = {sourceFile, clip->path().userData, clip->path().formatEntryClassName, (start - contentStart) / 1000, (end - start) / 1000};
        if (operation.type == AudioExtractionTask::Tempo && (!std::isfinite(start) || !std::isfinite(end) || start < 0 || end > std::numeric_limits<int>::max() || end - start < 1)) {
            showError(tr("The audio region exceeds the supported time range."));
            return;
        }
        qCDebug(lcAudioExtractionScenario) << "Entered progressing state";
        m_task->start(extractorIndex, operation.input, outputTrackCount, outputRoutes, compression);
    }

    void AudioExtractionAddOn::showError(const QString &message) {
        if (m_dialog)
            m_dialog->setProperty("scenarioError", message);
    }

    bool AudioExtractionAddOn::sourceUnchanged() const {
        if (!sourceAvailable())
            return false;
        const auto &operation = *m_operation;
        const auto source = operation.source.data();
        const auto window = windowHandle()->cast<Core::ProjectWindowInterface>();
        const auto timeline = window->projectTimeline()->musicTimeline();
        const QFileInfo file(operation.input.filePath);
        return source->clipSequence()->track() == operation.track && source->position() == operation.position
            && source->clipStart() == operation.clipStart && source->clipLength() == operation.clipLength
            && source->length() == operation.length && source->path() == operation.sourcePath
            && timeline->tempi() == operation.tempi && timeline->timeSignatures() == operation.timeSignatures
            && file.isFile() && file.size() == operation.fileSize && file.lastModified() == operation.fileModified;
    }

    void AudioExtractionAddOn::applyTempo() {
        if (!m_tempoPreview || !m_tempoPreview->valid())
            return;
        m_tempoPreview->pause();
        if (!sourceUnchanged()) {
            if (m_operation)
                m_operation->readLock.release();
            showError(tr("The audio clip or project timeline changed. Extract the tempo again."));
            return;
        }
        const auto &plan = m_tempoPreview->plan();
        auto &operation = *m_operation;
        if (!operation.readLock.acquire(operation.document->transactionController())) {
            qCWarning(lcAudioExtractionScenario) << "Could not acquire a document read lock.";
            windowHandle()->cast<Core::ProjectWindowInterface>()->sendOperationConflictNotification();
            return;
        }
        auto document = operation.document.data();
        auto model = document->model();
        auto sequence = model->tempos();
        auto source = operation.source.data();
        const bool clipChanged = source->position() != plan.position || source->clipStart() != plan.clipStart || source->clipLength() != plan.clipLength || source->length() != plan.length;
        if (plan.tempi == operation.tempi && !clipChanged) {
            QMetaObject::invokeMethod(m_dialog, "accept");
            return;
        }
        auto controller = document->transactionController();
        operation.readLock.release();
        const auto transaction = controller->beginTransaction();
        if (transaction == Core::TransactionController::TransactionId::Invalid) {
            qCWarning(lcAudioExtractionScenario) << "The extracted tempo transaction could not be started.";
            windowHandle()->cast<Core::ProjectWindowInterface>()->sendOperationConflictNotification();
            return;
        }
        qCDebug(lcAudioExtractionScenario) << "Entered committing state";
        const auto abort = [&] {
            qCDebug(lcAudioExtractionScenario) << "Entered aborting state";
            controller->abortTransaction(transaction);
            showError(tr("The extracted tempo could not be applied."));
        };
        const auto existing = sequence->slice(plan.begin, plan.end - plan.begin);
        for (auto item : existing) {
            if (!plan.tempi.contains(item->position())) {
                if (!sequence->removeItem(item) || !model->destroyItem(item)) {
                    abort();
                    return;
                }
            }
        }
        for (auto it = plan.tempi.lowerBound(plan.begin); it != plan.tempi.cend() && it.key() <= plan.end; ++it) {
            const auto items = sequence->slice(it.key(), 1);
            if (!items.isEmpty()) {
                items.first()->setValue(it.value());
            } else {
                auto item = model->createTempo();
                item->setPosition(it.key());
                item->setValue(it.value());
                if (!sequence->insertItem(item)) {
                    abort();
                    return;
                }
            }
        }
        source->setPosition(plan.position);
        source->setClipStart(plan.clipStart);
        source->setClipLength(plan.clipLength);
        source->setLength(plan.length);
        if (!controller->commitTransaction(transaction, tr("Extracting tempo"))) {
            abort();
            return;
        }
        qCDebug(lcAudioExtractionScenario) << "Entered completed state";
        QMetaObject::invokeMethod(m_dialog, "accept");
    }

    void AudioExtractionAddOn::finish() {
        if (!sourceAvailable()) {
            if (m_operation)
                m_operation->readLock.release();
            showError(tr("The original audio clip is no longer available."));
            return;
        }
        auto &operation = *m_operation;
        auto source = operation.source.data();
        auto windowInterface = windowHandle()->cast<Core::ProjectWindowInterface>();
        auto timeline = windowInterface->projectTimeline()->musicTimeline();
        if (source->clipSequence()->track() != operation.track || source->position() != operation.position
            || source->clipStart() != operation.clipStart || source->clipLength() != operation.clipLength
            || source->path() != operation.sourcePath || timeline->tempi() != operation.tempi) {
            operation.readLock.release();
            showError(tr("The original audio clip or project tempo changed during extraction. Try again."));
            return;
        }
        const auto &result = m_task->result();
        if (operation.type == AudioExtractionTask::Tempo) {
            if (!sourceUnchanged()) {
                operation.readLock.release();
                showError(tr("The audio clip or project timeline changed. Extract the tempo again."));
                return;
            }
            TempoExtractionInput input{result.beats, operation.tempi, operation.timeSignatures, timeline->ticksPerQuarterNote(), operation.position, operation.clipStart, operation.clipLength, operation.length,
                timeline->create(0, 0, operation.position).millisecond() / 1000, operation.input.durationSeconds};
            m_tempoPreview = new TempoExtractionPreview(input, operation.input, windowInterface, this);
            auto window = qobject_cast<QQuickWindow *>(windowInterface->window());
            QQmlComponent component(Core::RuntimeInterface::qmlEngine(), "DiffScope.Synth", "TempoExtractionPreviewDialog");
            if (component.isError())
                qFatal() << component.errorString();
            auto dialog = component.createWithInitialProperties({
                {QStringLiteral("parent"), QVariant::fromValue(window->contentItem())},
                {QStringLiteral("addOn"), QVariant::fromValue(this)},
                {QStringLiteral("preview"), QVariant::fromValue(m_tempoPreview.data())},
            });
            if (!dialog)
                qFatal() << component.errorString();
            m_pendingDialog = dialog;
            m_tempoPreview->setParent(dialog);
            UIShell::DialogPositioner::positionPopup(dialog, window);
            QMetaObject::invokeMethod(m_dialog, "accept");
            return;
        }
        qCDebug(lcAudioExtractionScenario) << "Entered committing state";
        dspx::Clip *firstClip = nullptr;
        const bool committed = result.embeddedStems.isEmpty()
            ? commitExtractedTracks(result, firstClip)
            : commitEmbeddedSeparation(result, firstClip);
        if (!committed)
            return;
        if (firstClip)
            operation.document->selectionModel()->select(firstClip, dspx::SelectionModel::Select | dspx::SelectionModel::SetCurrentItem | dspx::SelectionModel::ClearPreviousSelection);
        qCDebug(lcAudioExtractionScenario) << "Entered completed state";
        QMetaObject::invokeMethod(m_dialog, "accept");
    }

    bool AudioExtractionAddOn::commitExtractedTracks(const AudioExtractionResult &result, dspx::Clip *&firstClip) {
        auto &operation = *m_operation;
        auto windowInterface = windowHandle()->cast<Core::ProjectWindowInterface>();
        auto document = operation.document.data();
        auto controller = document->transactionController();
        operation.readLock.release();
        const auto transaction = controller->beginTransaction();
        if (transaction == Core::TransactionController::TransactionId::Invalid) {
            qCWarning(lcAudioExtractionScenario) << "The extracted track insertion transaction could not be started.";
            windowInterface->sendOperationConflictNotification();
            return false;
        }

        QTemporaryDir backups(temporaryDirectoryPath(QStringLiteral("extraction-backup")));
        QStringList writtenFiles;
        QMap<QString, QString> backupFiles;
        const auto abort = [&](const QString &message) {
            qCDebug(lcAudioExtractionScenario) << "Entered aborting state";
            controller->abortTransaction(transaction);
            QStringList failedRestores;
            for (const auto &file : writtenFiles) {
                const bool restored = backupFiles.contains(file) ? copyFile(backupFiles.value(file), file) : QFile::remove(file);
                if (!restored)
                    failedRestores.append(normalizedPath(file));
            }
            if (!failedRestores.isEmpty()) {
                backups.setAutoRemove(false);
                showError(tr("%1\nSome files could not be restored. Backups are in %2.\n%3").arg(message, normalizedPath(backups.path()), failedRestores.join(u'\n')));
            } else {
                showError(message);
            }
        };

        QList<dspx::AudioPathInfo> paths;
        if (operation.type == AudioExtractionTask::Separation) {
            QSet<QString> names;
            QStringList destinations;
            for (const auto &stem : result.stems) {
                const auto name = safeName(operation.name) + u'_' + safeName(stem.fileNameSuffix) + QStringLiteral(".wav");
                if (names.contains(name.toCaseFolded())) {
                    abort(tr("The separated track names produce duplicate file names."));
                    return false;
                }
                names.insert(name.toCaseFolded());
                const auto target = QDir(operation.directory).absoluteFilePath(name);
                QFileInfo existing(target);
                if (existing.exists() || existing.isSymLink()) {
                    const auto approved = operation.overwriteFiles.constFind(fileKey(target));
                    if (existing.isSymLink() || !existing.isFile() || approved == operation.overwriteFiles.cend()
                        || approved->first != existing.size() || approved->second != existing.lastModified()) {
                        abort(tr("An output file appeared or changed during extraction. Start again to confirm overwriting it."));
                        return false;
                    }
                    const auto backup = backups.filePath(QString::number(backupFiles.size()) + QStringLiteral(".wav"));
                    if (!backups.isValid() || !QFile::copy(target, backup)) {
                        abort(tr("Could not back up an existing output file."));
                        return false;
                    }
                    backupFiles.insert(target, backup);
                }
                destinations.append(target);
            }
            for (qsizetype index = 0; index < result.stems.size(); ++index) {
                const auto target = destinations.at(index);
                if (!copyFile(result.stems.at(index).filePath, target)) {
                    abort(tr("Could not save the separated audio files."));
                    return false;
                }
                writtenFiles.append(target);
                const QFileInfo info(target);
                dspx::AudioPathInfo path;
                path.absoluteDir = info.absolutePath();
                path.fileName = info.fileName();
                path.digest = result.stems.at(index).digest;
                const auto locker = windowInterface->projectDocumentContext()->fileLocker();
                if (locker && !locker->path().isEmpty()) {
                    const auto projectDirectory = QFileInfo(locker->path()).absoluteDir();
                    const auto relative = projectDirectory.relativeFilePath(info.absoluteFilePath());
                    if (!QDir::isAbsolutePath(relative) && relative != QStringLiteral("..") && !relative.startsWith(QStringLiteral("../")))
                        path.relativeDir = projectDirectory.relativeFilePath(info.absolutePath());
                }
                paths.append(path);
            }
        }

        if (const auto error = insertExtractedClips(result, paths, firstClip); !error.isEmpty()) {
            abort(error);
            return false;
        }
        const auto description = operation.type == AudioExtractionTask::Notes ? tr("Extracting notes")
            : operation.type == AudioExtractionTask::Pitch ? tr("Extracting pitch")
            : tr("Separating audio");
        if (!controller->commitTransaction(transaction, description)) {
            abort(tr("Could not commit the extracted tracks."));
            return false;
        }
        return true;
    }

    bool AudioExtractionAddOn::commitEmbeddedSeparation(const AudioExtractionResult &result, dspx::Clip *&firstClip) {
        using EmbeddedAudio = Audio::EmbeddedAudio;

        const auto window = QPointer(windowHandle()->cast<Core::ProjectWindowInterface>());
        const auto store = QPointer(EmbeddedAudio::of(window->projectDocumentContext()));
        QList<EmbeddedAudio::Conflict> conflicts;
        for (const auto &audio : result.embeddedStems)
            conflicts.append(store->conflict(audio));
        const bool downgrade = std::ranges::any_of(conflicts, [](const auto &conflict) {
            return conflict.action == EmbeddedAudio::Conflict::Downgrade;
        });
        if (downgrade) {
            const auto guard = QPointer(this);
            const auto task = m_task;
            const auto answer = SVS::MessageBox::warning(Core::RuntimeInterface::qmlEngine(), window->window(),
                tr("Replace Lossless Audio"),
                tr("Some separated audio matches lossless audio in this project. Replace the lossless copies with lossy copies for all clips that use them?"),
                SVS::SVSCraft::Yes | SVS::SVSCraft::Cancel, SVS::SVSCraft::Cancel);
            if (!guard || !task || m_task != task)
                return false;
            if (answer != SVS::SVSCraft::Yes || !window || !store || !sourceAvailable()) {
                m_operation->readLock.release();
                return false;
            }
        }

        m_operation->readLock.release();
        QString error;
        const auto status = store->apply(result.embeddedStems, conflicts, tr("Separating audio"), [&](const QList<dspx::AudioPathInfo> &paths) {
            error = insertExtractedClips(result, paths, firstClip);
            return error.isEmpty();
        }, downgrade);
        if (status == EmbeddedAudio::ApplyResult::Busy || status == EmbeddedAudio::ApplyResult::Stale) {
            window->sendOperationConflictNotification();
        } else if (status != EmbeddedAudio::ApplyResult::Applied) {
            showError(error.isEmpty() ? tr("Could not commit the extracted tracks.") : error);
        }
        return status == EmbeddedAudio::ApplyResult::Applied;
    }

    QString AudioExtractionAddOn::insertExtractedClips(const AudioExtractionResult &result, const QList<dspx::AudioPathInfo> &paths, dspx::Clip *&firstClip) {
        const auto &operation = *m_operation;
        auto model = operation.document->model();
        auto tracks = model->tracks();
        auto timeline = windowHandle()->cast<Core::ProjectWindowInterface>()->projectTimeline()->musicTimeline();
        const bool singingExtraction = operation.type == AudioExtractionTask::Notes || operation.type == AudioExtractionTask::Pitch;
        const auto count = singingExtraction ? 1 : result.stems.size();
        int insertionIndex = tracks->items().indexOf(operation.track) + 1;
        const auto startMsec = timeline->create(0, 0, operation.position).millisecond();
        for (qsizetype index = 0; index < count; ++index) {
            const auto name = operation.type == AudioExtractionTask::Notes
                ? tr("%1 (Extracted notes)").arg(operation.name)
                : operation.type == AudioExtractionTask::Pitch
                    ? tr("%1 (Extracted pitch)").arg(operation.name)
                    : tr("%1 (%2)").arg(operation.name, result.stems.at(index).name);
            auto track = model->createTrack();
            track->fromOpenDSPX(opendspx::Track{});
            track->setName(name);
            track->setColorId(operation.track->colorId());
            if (!tracks->insertItem(insertionIndex++, track))
                return tr("Could not insert the extracted tracks.");
            dspx::Clip *clip;
            if (singingExtraction) {
                auto singing = model->createSingingClip();
                Core::SingerSettings::applyDefaultSinger(singing);
                clip = singing;
                if (operation.type == AudioExtractionTask::Pitch) {
                    auto parameter = model->createParameter();
                    if (!singing->parameters()->insertItem(QStringLiteral("pitch"), parameter)
                        || !parameter->freeEdited()->splice(0, 0, resamplePitch(result, timeline, operation.position, operation.clipLength)))
                        return tr("Could not insert the extracted pitch.");
                } else {
                    auto lyrics = Core::CoreInterface::defaultLyricManager();
                    const auto lyric = lyrics->getDefaultLyricForSingingClip(singing);
                    for (const auto &extracted : result.notes) {
                        const double startSeconds = std::clamp(extracted.startSeconds, 0.0, operation.input.durationSeconds);
                        const double endSeconds = std::clamp(extracted.startSeconds + extracted.durationSeconds, 0.0, operation.input.durationSeconds);
                        const int start = std::clamp(timeline->create(startMsec + startSeconds * 1000).totalTick() - operation.position, 0, operation.clipLength);
                        const int end = std::clamp(timeline->create(startMsec + endSeconds * 1000).totalTick() - operation.position, 0, operation.clipLength);
                        if (end <= start)
                            continue;
                        auto note = model->createNote();
                        const int key = std::clamp(qRound(extracted.cent / 100.0), 0, 127);
                        note->setKeyNumber(key);
                        note->setCentShift(std::clamp(extracted.cent - key * 100, -50, 50));
                        note->setPosition(start);
                        note->setLength(end - start);
                        note->setLyric(lyric);
                        note->setLanguage(singing->defaultLanguage());
                        if (!singing->notes()->insertItem(note))
                            return tr("Could not insert the extracted notes.");
                    }
                }
            } else {
                auto audio = model->createAudioClip();
                audio->setPath(paths.at(index));
                clip = audio;
            }
            clip->setName(name);
            clip->setPosition(operation.position);
            clip->setClipStart(0);
            clip->setClipLength(operation.clipLength);
            clip->setLength(operation.clipLength);
            clip->setGain(1);
            if (!track->clips()->insertItem(clip))
                return tr("Could not insert the extracted clips.");
            if (!firstClip)
                firstClip = clip;
        }
        return {};
    }

    void AudioExtractionAddOn::closeDialog() {
        if (m_pendingDialog) {
            if (m_task) {
                m_task->cancel();
                m_task = nullptr;
            }
            m_dialog->deleteLater();
            m_dialog = m_pendingDialog;
            m_pendingDialog = nullptr;
            QMetaObject::invokeMethod(m_dialog, "open", Qt::QueuedConnection);
            qCDebug(lcAudioExtractionScenario) << "Entered previewing state";
            return;
        }
        if (m_tempoPreview) {
            m_tempoPreview->pause();
            m_tempoPreview = nullptr;
        }
        if (m_task) {
            m_task->cancel();
            if (!m_dialog)
                m_task->deleteLater();
            m_task = nullptr;
        }
        if (m_dialog) {
            m_dialog->deleteLater();
            m_dialog = nullptr;
        }
        m_operation.reset();
        qCDebug(lcAudioExtractionScenario) << "Entered idle state";
    }

}

#include "moc_AudioExtractionAddOn.cpp"
