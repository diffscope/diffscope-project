#include "ExportAudioAddOn.h"

#include <algorithm>
#include <memory>
#include <utility>

#include <QCoreApplication>
#include <QDir>
#include <QEventLoop>
#include <QFileDialog>
#include <QFileInfo>
#include <QPair>
#include <QQmlComponent>
#include <QStandardPaths>
#include <QTimer>
#include <QVariant>
#include <QWindow>

#include <CoreApi/filelocker.h>
#include <CoreApi/runtimeinterface.h>

#include <SVSCraftCore/MusicTime.h>
#include <SVSCraftCore/MusicTimeline.h>

#include <TalcsFormat/AbstractAudioFormatIO.h>
#include <TalcsFormat/FormatManager.h>

#include <QAKQuick/quickactioncontext.h>

#include <dspxmodel/Clip.h>
#include <dspxmodel/ClipSequence.h>
#include <dspxmodel/ClipTime.h>
#include <dspxmodel/Model.h>
#include <dspxmodel/Timeline.h>
#include <dspxmodel/Track.h>
#include <dspxmodel/TrackList.h>

#include <coreplugin/DspxDocument.h>
#include <coreplugin/NotificationMessage.h>
#include <coreplugin/ProjectDocumentContext.h>
#include <coreplugin/ProjectTimeline.h>
#include <coreplugin/ProjectWindowInterface.h>

#include <audio/AudioExporter.h>
#include <audio/AudioExporterConfig.h>
#include <audio/GlobalAudioContext.h>
#include <audio/PreviewSoundPlayer.h>
#include <audio/internal/AudioExporterPresets.h>
#include <audio/internal/AudioPreference.h>
#include <audio/private/AudioExporter_p.h>

namespace Audio::Internal {
    namespace {
        QString documentsDirectory() {
            const auto locations = QStandardPaths::standardLocations(QStandardPaths::DocumentsLocation);
            return locations.isEmpty() ? QDir::homePath() : locations.first();
        }

        QString projectDirectory(Core::ProjectWindowInterface *windowInterface) {
            const auto projectDocumentContext = windowInterface->projectDocumentContext();
            if (projectDocumentContext->fileLocker()) {
                const QFileInfo fileInfo(projectDocumentContext->fileLocker()->path());
                if (!fileInfo.filePath().isEmpty() && !fileInfo.dir().isRelative()) {
                    return fileInfo.dir().path();
                }
            }
            return documentsDirectory();
        }

        void applyFileType(AudioExporterConfig &config, int index) {
            const QFileInfo fileInfo(config.fileName());
            config.setFileType(static_cast<AudioExporterConfig::FileType>(index));
            config.setFormatOption(0);
            config.setFileName(fileInfo.completeBaseName() + QStringLiteral(".") +
                               AudioExporterConfig::extensionOfType(static_cast<AudioExporterConfig::FileType>(index)));
        }
    }

    ExportAudioProgressController::ExportAudioProgressController(QObject *parent) : QObject(parent) {
    }

    void ExportAudioProgressController::setActionRequestedCallback(std::function<void()> callback) {
        m_actionRequestedCallback = std::move(callback);
    }

    void ExportAudioProgressController::setCloseRequestedCallback(std::function<void()> callback) {
        m_closeRequestedCallback = std::move(callback);
    }

    void ExportAudioProgressController::requestAction() {
        if (m_actionRequestedCallback) {
            m_actionRequestedCallback();
        }
    }

    void ExportAudioProgressController::requestClose() {
        if (m_closeRequestedCallback) {
            m_closeRequestedCallback();
        }
    }

    ExportAudioAddOn::ExportAudioAddOn(QObject *parent) : WindowInterfaceAddOn(parent) {
        m_simpleConfig.setFormatQuality(100);
        m_simpleConfig.setFormatSampleRate(48000);
    }

    ExportAudioAddOn::~ExportAudioAddOn() = default;

    void ExportAudioAddOn::initialize() {
        auto windowInterface = windowHandle()->cast<Core::ProjectWindowInterface>();
        windowInterface->addObject(this);
        m_exportCompletedMessage = new Core::NotificationMessage(windowInterface->window());
        m_exportCompletedMessage->setTitle(tr("Audio export completed"));
        m_exportCompletedMessage->setIcon(SVS::SVSCraft::Success);
        m_exportCompletedMessage->setAllowDoNotShowAgain(true);
        m_exportCompletedMessage->setDoNotShowAgainIdentifier("org.diffscope.audio.exportaudioaddon.message");

        auto io = GlobalAudioContext::formatManager()->getFormatLoad(":/diffscope/audio/soundfx/export_completed.ogg");
        Q_ASSERT(io);
        m_completedSound = new PreviewSoundPlayer(io, this);

        QQmlComponent component(Core::RuntimeInterface::qmlEngine(), "DiffScope.Audio", "ExportAudioAddOnActions");
        if (component.isError()) {
            qFatal() << component.errorString();
        }
        auto o = component.createWithInitialProperties({
            {"addOn", QVariant::fromValue(this)},
        });
        o->setParent(this);
        QMetaObject::invokeMethod(o, "registerToContext", windowInterface->actionContext());
    }

    void ExportAudioAddOn::extensionsInitialized() {
    }

    bool ExportAudioAddOn::delayedInitialize() {
        return WindowInterfaceAddOn::delayedInitialize();
    }

    ExportAudioAddOn *ExportAudioAddOn::of(Core::ProjectWindowInterface *windowHandle) {
        return windowHandle->getFirstObject<ExportAudioAddOn>();
    }

    AudioExporterParameter ExportAudioAddOn::currentParameter() const {
        return m_currentParameter;
    }

    void ExportAudioAddOn::setCurrentParameter(const AudioExporterParameter &parameter) {
        if (m_currentParameter == parameter) {
            return;
        }
        m_currentParameter = parameter;
        emit currentParameterChanged();
    }

    AudioExporterConfig ExportAudioAddOn::simpleConfig() const {
        return m_simpleConfig;
    }

    void ExportAudioAddOn::setSimpleConfig(const AudioExporterConfig &config) {
        if (m_simpleConfig == config)
            return;
        m_simpleConfig = config;
        emit simpleConfigChanged();
    }

    void ExportAudioAddOn::exportAudio() {
        auto windowInterface = windowHandle()->cast<Core::ProjectWindowInterface>();
        if (!windowInterface || !windowInterface->window())
            return;

        AudioExporter exporter(windowInterface, this);
        auto model = windowInterface->projectDocumentContext()->document()->model();
        auto timeRangeAllEnd = std::ranges::max(std::views::transform(model->tracks()->items(), [](dspx::Track *track) {
            return track->clips()->size() == 0 ? 0 : std::ranges::max(std::views::transform(track->clips()->asRange(), [](dspx::Clip *clip) {
                return clip->position() + clip->time()->clipLen();
            }));
        }));
        auto timeline = model->timeline();
        auto timeRangeLoopSectionStart = 0;
        auto timeRangeLoopSectionEnd = timeRangeAllEnd;
        if (timeline->isLoopEnabled() && timeline->loopLength() > 0) {
            timeRangeLoopSectionStart = timeline->loopStart();
            timeRangeLoopSectionEnd = timeRangeLoopSectionStart + timeline->loopLength();
        }
        m_currentParameter.setRangeStart(0);
        m_currentParameter.setRangeLength(timeRangeAllEnd);

        connect(this, &ExportAudioAddOn::currentParameterChanged, &exporter, [this, &exporter] {
            exporter.setParameter(currentParameter());
        });
        exporter.setParameter(currentParameter());

        if (m_simpleConfig.fileName().isEmpty() || m_simpleConfig.fileDirectory().isEmpty()) {
            m_simpleConfig.setFileDirectory(AudioExporterPrivate::of(&exporter)->projectDirectory());
            m_simpleConfig.setFileName(AudioExporterPrivate::of(&exporter)->projectName() + ".wav");
        }

        QQmlComponent component(Core::RuntimeInterface::qmlEngine(), "DiffScope.Audio", "AudioExportDialog");
        if (component.isError()) {
            qFatal() << component.errorString();
        }
        std::unique_ptr<QWindow> dialog(qobject_cast<QWindow *>(component.createWithInitialProperties({
            {"addOn", QVariant::fromValue(this)},
            {"exporter", QVariant::fromValue(&exporter)},
            {"timeRangeAllEnd", timeRangeAllEnd},
            {"timeRangeLoopSectionStart", timeRangeLoopSectionStart},
            {"timeRangeLoopSectionEnd", timeRangeLoopSectionEnd},
        })));
        if (!dialog) {
            qFatal() << component.errorString();
        }
        dialog->setTransientParent(windowInterface->window());
        dialog->show();

        QEventLoop eventLoop;
        connect(dialog.get(), SIGNAL(finished()), &eventLoop, SLOT(quit()));
        eventLoop.exec();
    }

    QStringList ExportAudioAddOn::formatOptions(int fileType) {
        return AudioExporterConfig::formatOptionsOfType(static_cast<AudioExporterConfig::FileType>(fileType));
    }

    void ExportAudioAddOn::browseFile() {
        auto presets = AudioExporterPresets::instance();
        auto config = presets->currentConfig();
        auto windowInterface = windowHandle()->cast<Core::ProjectWindowInterface>();
        const QStringList filters = {
            tr("WAV (*.wav)"),
            tr("FLAC (*.flac)"),
            tr("Ogg Vorbis (*.ogg)"),
            tr("MP3 (*.mp3)"),
        };
        QString selectedFilter = filters.at(config.fileType());
        const auto path = QFileDialog::getSaveFileName(
            nullptr,
            {},
            QDir(projectDirectory(windowInterface)).absoluteFilePath(config.fileDirectory()),
            filters.join(QStringLiteral(";;")),
            &selectedFilter
        );
        if (path.isEmpty()) {
            return;
        }

        const QFileInfo fileInfo(path);
        const auto templateSuffix = config.mixingOption() == AudioExporterConfig::MO_Mixed
            ? QStringLiteral(".")
            : QStringLiteral("_${trackIndex}_${trackName}.");
        config.setFileName(fileInfo.completeBaseName() + templateSuffix + fileInfo.suffix());
        config.setFileDirectory(fileInfo.dir().canonicalPath());
        applyFileType(config, filters.indexOf(selectedFilter));
        presets->setCurrentConfig(config);
    }

    void ExportAudioAddOn::browseFileSimple() {
        auto windowInterface = windowHandle()->cast<Core::ProjectWindowInterface>();
        auto config = m_simpleConfig;
        const QStringList filters = {
            tr("WAV (*.wav)"),
            tr("Ogg Vorbis (*.ogg)"),
        };
        QString selectedFilter = filters.at(config.fileType() == AudioExporterConfig::FT_Wav ? 0 : 1);
        const auto path = QFileDialog::getSaveFileName(
            nullptr,
            {},
            calculateSimplePath(config),
            filters.join(QStringLiteral(";;")),
            &selectedFilter
        );
        if (path.isEmpty()) {
            return;
        }
        const QFileInfo fileInfo(path);
        const auto templateSuffix = config.mixingOption() == AudioExporterConfig::MO_Mixed
            ? QStringLiteral(".")
            : QStringLiteral("_${trackIndex}_${trackName}.");
        config.setFileName(fileInfo.completeBaseName() + templateSuffix + fileInfo.suffix());
        config.setFileDirectory(fileInfo.dir().canonicalPath());
        applyFileType(config, filters.indexOf(selectedFilter) == 0 ? AudioExporterConfig::FT_Wav : AudioExporterConfig::FT_OggVorbis);
        setSimpleConfig(config);
    }

    void ExportAudioAddOn::setMixingOption(int index) {
        auto presets = AudioExporterPresets::instance();
        auto config = presets->currentConfig();
        config.setMixingOption(static_cast<AudioExporterConfig::MixingOption>(index));

        const QFileInfo fileInfo(config.fileName());
        auto basename = fileInfo.completeBaseName();
        auto suffix = fileInfo.suffix();
        if (index == AudioExporterConfig::MO_Mixed) {
            if (basename.endsWith(QStringLiteral("_${trackIndex}_${trackName}"))) {
                basename = basename.chopped(27);
            }
        } else if (!basename.contains(QStringLiteral("${trackIndex}")) &&
                   !basename.contains(QStringLiteral("${trackName}"))) {
            basename += QStringLiteral("_${trackIndex}_${trackName}");
        }
        if (suffix.isEmpty()) {
            suffix = AudioExporterConfig::extensionOfType(config.fileType());
        }
        config.setFileName(basename + QStringLiteral(".") + suffix);
        presets->setCurrentConfig(config);
    }

    void ExportAudioAddOn::setFileType(int index) {
        auto presets = AudioExporterPresets::instance();
        auto config = presets->currentConfig();
        applyFileType(config, index);
        presets->setCurrentConfig(config);
    }

    void ExportAudioAddOn::setMixingOptionSimple(int index) {
        auto config = m_simpleConfig;
        config.setMixingOption(static_cast<AudioExporterConfig::MixingOption>(index));

        const QFileInfo fileInfo(config.fileName());
        auto basename = fileInfo.completeBaseName();
        auto suffix = fileInfo.suffix();
        if (index == AudioExporterConfig::MO_Mixed) {
            if (basename.endsWith(QStringLiteral("_${trackIndex}_${trackName}"))) {
                basename = basename.chopped(27);
            }
        } else if (!basename.contains(QStringLiteral("${trackIndex}")) &&
                   !basename.contains(QStringLiteral("${trackName}"))) {
            basename += QStringLiteral("_${trackIndex}_${trackName}");
                   }
        if (suffix.isEmpty()) {
            suffix = AudioExporterConfig::extensionOfType(config.fileType());
        }
        config.setFileName(basename + QStringLiteral(".") + suffix);
        setSimpleConfig(config);
    }

    void ExportAudioAddOn::setFileTypeSimple(int index) {
        auto config = m_simpleConfig;
        applyFileType(config, index);
        setSimpleConfig(config);
    }

    double ExportAudioAddOn::calculateDurationInMsec(AudioExporter *exporter) const {
        auto windowInterface = windowHandle()->cast<Core::ProjectWindowInterface>();

        auto range = AudioExporterPrivate::of(exporter)->calculateRange();
        auto musicTimeline = windowInterface->projectTimeline()->musicTimeline();

        return musicTimeline->create(0, 0, range.second).millisecond() - musicTimeline->create(0, 0, range.first).millisecond();
    }

    QString ExportAudioAddOn::calculateSimplePath(const AudioExporterConfig &config) {
        auto fileName = config.fileName();
        if (config.mixingOption() == AudioExporterConfig::MO_Mixed) {
            return QDir::toNativeSeparators(QDir(config.fileDirectory()).absoluteFilePath(fileName));
        } else {
            QFileInfo info(fileName);
            auto baseName = info.completeBaseName();
            Q_ASSERT(baseName.endsWith(QStringLiteral("_${trackIndex}_${trackName}")));
            baseName = baseName.chopped(27);
            return QDir::toNativeSeparators(QDir(config.fileDirectory()).absoluteFilePath(baseName + QStringLiteral(".") + info.suffix()));
        }
    }

    void ExportAudioAddOn::appendFileNameTemplate(const QString &templateString) {
        auto config = AudioExporterPresets::instance()->currentConfig();
        QFileInfo info(config.fileName());
        config.setFileName(info.completeBaseName() + "_" + templateString + "." + info.suffix());
        AudioExporterPresets::instance()->setCurrentConfig(config);
    }

    bool ExportAudioAddOn::runExport(AudioExporter *exporter) {
        if (!exporter) {
            return false;
        }

        auto windowInterface = windowHandle()->cast<Core::ProjectWindowInterface>();
        if (!windowInterface || !windowInterface->window()) {
            return false;
        }

        const auto sendExportCompletedNotification = [this, windowInterface] {
            m_exportCompletedMessage->close();
            windowInterface->sendNotification(m_exportCompletedMessage, Core::ProjectWindowInterface::AutoHide);
            if (AudioPreference::shouldPlayNotificationSoundWhenExportCompleted()) {
                m_completedSound->play();
            }
        };

        QQmlComponent component(Core::RuntimeInterface::qmlEngine(), "DiffScope.Audio", "AudioExportProgressDialog");
        if (component.isError()) {
            qFatal() << component.errorString();
        }
        std::unique_ptr<QWindow> progressDialog(qobject_cast<QWindow *>(component.create()));
        if (!progressDialog) {
            qFatal() << component.errorString();
        }
        progressDialog->setTransientParent(windowInterface->window());

        auto setDialogProperty = [&progressDialog](const char *name, const QVariant &value) {
            progressDialog->setProperty(name, value);
        };
        auto closeDialog = [&progressDialog] {
            progressDialog->setProperty("closeAllowed", true);
            QMetaObject::invokeMethod(progressDialog.get(), "done");
        };

        const auto setProgressStatus = [&](ExportAudioProgressController::ProgressStatus status) {
            setDialogProperty("progressStatus", static_cast<int>(status));
        };
        const auto setActionStatus = [&](ExportAudioProgressController::ActionStatus status) {
            setDialogProperty("actionStatus", static_cast<int>(status));
        };

        int alertRevision = 0;
        const auto requestAlert = [&] {
            setDialogProperty("alertRevision", ++alertRevision);
        };

        setProgressStatus(ExportAudioProgressController::Preparing);
        setDialogProperty("progressValue", 0.0);
        setActionStatus(ExportAudioProgressController::CancelAction);
        setDialogProperty("alertRevision", alertRevision);
        setDialogProperty("runtimeWarningCount", 0);
        setDialogProperty("keepPartialFiles", false);
        setDialogProperty("messages", QVariantList{});

        bool isProgressing = false;
        bool isExporting = true;
        bool finishedWithoutRuntimeWarnings = false;
        QVariantList messages;
        int runtimeWarningCount = 0;
        QHash<int, double> progressRatioHash;

        auto appendMessage = [&](ExportAudioProgressController::MessageType type, const QString &message = {}, int trackNumber = -1, const QString &trackName = {}) {
            QVariantMap item;
            item.insert(QStringLiteral("type"), static_cast<int>(type));
            item.insert(QStringLiteral("text"), message);
            item.insert(QStringLiteral("trackNumber"), trackNumber);
            item.insert(QStringLiteral("trackName"), trackName);
            messages.append(item);
            setDialogProperty("messages", messages);
            if (type != ExportAudioProgressController::RuntimeErrorMessage) {
                ++runtimeWarningCount;
                setDialogProperty("runtimeWarningCount", runtimeWarningCount);
            }
        };

        const auto sourceIndexes = AudioExporterPrivate::of(exporter)->sourceIndexes();
        const auto sourceInfo = [exporter, sourceIndexes](int sourceIndex) -> QPair<int, QString> {
            if (sourceIndex >= 0 && sourceIndex < sourceIndexes.size()) {
                const auto trackIndex = sourceIndexes.at(sourceIndex);
                return {trackIndex + 1, AudioExporterPrivate::of(exporter)->trackName(trackIndex)};
            }
            return {sourceIndex + 1, {}};
        };

        if (exporter->config().mixingOption() == AudioExporterConfig::MO_Mixed) {
            connect(exporter, &AudioExporter::progressChanged, progressDialog.get(), [setDialogProperty, setProgressStatus, &isProgressing](double ratio) {
                if (!isProgressing) {
                    isProgressing = true;
                    setProgressStatus(ExportAudioProgressController::Exporting);
                }
                setDialogProperty("progressValue", ratio);
            });
        } else {
            const auto sourceCount = std::max<qsizetype>(1, exporter->fileList().size());
            connect(exporter, &AudioExporter::progressChanged, progressDialog.get(),
                    [sourceCount, setDialogProperty, setProgressStatus, &progressRatioHash, &isProgressing](double ratio, int sourceIndex) {
                if (!isProgressing) {
                    isProgressing = true;
                    setProgressStatus(ExportAudioProgressController::Exporting);
                }
                progressRatioHash[sourceIndex] = ratio;
                double totalRatio = 0;
                for (const auto value : progressRatioHash.values()) {
                    totalRatio += value;
                }
                setDialogProperty("progressValue", totalRatio / sourceCount);
            });
        }

        connect(exporter, &AudioExporter::clippingDetected, progressDialog.get(), [&, sourceInfo](int sourceIndex) {
            if (sourceIndex == -1) {
                appendMessage(ExportAudioProgressController::ClippingDetectedMessage);
            } else {
                const auto labelParts = sourceInfo(sourceIndex);
                appendMessage(ExportAudioProgressController::ClippingDetectedMessage, {}, labelParts.first, labelParts.second);
            }
            requestAlert();
        });
        connect(exporter, &AudioExporter::runtimeWarningAdded, progressDialog.get(), [&](const QString &message, int sourceIndex) {
            Q_UNUSED(sourceIndex)
            appendMessage(ExportAudioProgressController::RuntimeWarningMessage, message);
            requestAlert();
        });

        ExportAudioProgressController progressController(progressDialog.get());
        progressController.setActionRequestedCallback([&] {
            if (isExporting) {
                exporter->cancel();
            } else {
                closeDialog();
            }
        });
        progressController.setCloseRequestedCallback([&] {
            if (isExporting) {
                exporter->cancel();
            } else {
                closeDialog();
            }
        });
        connect(progressDialog.get(), SIGNAL(actionRequested()), &progressController, SLOT(requestAction()));
        connect(progressDialog.get(), SIGNAL(closeRequested()), &progressController, SLOT(requestClose()));

        QEventLoop eventLoop;
        connect(progressDialog.get(), SIGNAL(finished()), &eventLoop, SLOT(quit()));

        progressDialog->show();

        QTimer::singleShot(0, progressDialog.get(), [&] {
            QCoreApplication::processEvents();
            const auto result = exporter->exec();
            isExporting = false;
            setActionStatus(ExportAudioProgressController::CloseAction);

            if (runtimeWarningCount > 0) {
                switch (result) {
                    case AudioExporter::R_Ok:
                        setProgressStatus(ExportAudioProgressController::FinishedWithWarnings);
                        sendExportCompletedNotification();
                        requestAlert();
                        break;
                    case AudioExporter::R_Abort:
                        setProgressStatus(ExportAudioProgressController::AbortedWithWarnings);
                        requestAlert();
                        break;
                    case AudioExporter::R_Fail:
                        setProgressStatus(ExportAudioProgressController::FailedWithWarnings);
                        appendMessage(ExportAudioProgressController::RuntimeErrorMessage, exporter->errorString());
                        requestAlert();
                        break;
                }
            } else {
                switch (result) {
                    case AudioExporter::R_Ok:
                        finishedWithoutRuntimeWarnings = true;
                        sendExportCompletedNotification();
                        closeDialog();
                        break;
                    case AudioExporter::R_Abort:
                        setProgressStatus(ExportAudioProgressController::Aborted);
                        requestAlert();
                        break;
                    case AudioExporter::R_Fail:
                        setProgressStatus(ExportAudioProgressController::Failed);
                        appendMessage(ExportAudioProgressController::RuntimeErrorMessage, exporter->errorString());
                        requestAlert();
                        break;
                }
            }
        });

        eventLoop.exec();

        if (!progressDialog->property("keepPartialFiles").toBool()) {
            exporter->cleanUp();
        }

        return finishedWithoutRuntimeWarnings;
    }

}

#include "moc_ExportAudioAddOn.cpp"
