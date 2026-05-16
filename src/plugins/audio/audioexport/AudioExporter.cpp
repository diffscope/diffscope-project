#include "AudioExporter.h"
#include "AudioExporter_p.h"

#include <algorithm>
#include <memory>
#include <utility>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QSet>
#include <QStandardPaths>
#include <QUuid>

#include <CoreApi/filelocker.h>

#include <TalcsCore/MixerAudioSource.h>
#include <TalcsCore/TransportAudioSource.h>
#include <TalcsDspx/DspxProjectAudioExporter.h>
#include <TalcsDspx/DspxProjectContext.h>
#include <TalcsDspx/DspxTrackContext.h>
#include <TalcsFormat/AudioFormatIO.h>

#include <audio/ProjectAudioContext.h>
#include <audio/private/ProjectAudioContext_p.h>
#include <audio/internal/AudioPreference.h>

#include <coreplugin/DspxDocument.h>
#include <coreplugin/ProjectDocumentContext.h>
#include <coreplugin/ProjectTimeline.h>
#include <coreplugin/ProjectWindowInterface.h>

#include <dspxmodel/Model.h>
#include <dspxmodel/SelectionModel.h>
#include <dspxmodel/Timeline.h>
#include <dspxmodel/Track.h>
#include <dspxmodel/TrackList.h>
#include <dspxmodel/TrackSelectionModel.h>

namespace Audio {

    namespace {

        template<class F>
        class ScopeGuard {
        public:
            explicit ScopeGuard(F &&function) : m_function(std::forward<F>(function)) {
            }

            ~ScopeGuard() {
                if (m_active) {
                    m_function();
                }
            }

            void dismiss() {
                m_active = false;
            }

        private:
            F m_function;
            bool m_active{true};
        };

        template<class F>
        ScopeGuard<F> makeScopeGuard(F &&function) {
            return ScopeGuard<F>(std::forward<F>(function));
        }

        static QList<AudioExporterListener *> s_listeners;

        static QString fallbackProjectName() {
            return AudioExporter::tr("Untitled");
        }

        static QString documentsDirectory() {
            const auto locations = QStandardPaths::standardLocations(QStandardPaths::DocumentsLocation);
            return locations.isEmpty() ? QDir::homePath() : locations.first();
        }

    }

    QString AudioExporterPrivate::projectName() const {
        const auto projectDocumentContext = windowHandle->projectDocumentContext();
        Q_ASSERT(projectDocumentContext);

        QString path;
        if (projectDocumentContext->fileLocker()) {
            path = projectDocumentContext->fileLocker()->path();
        }
        if (path.isEmpty()) {
            path = projectDocumentContext->defaultDocumentName();
        }

        const auto baseName = QFileInfo(path).baseName();
        return baseName.isEmpty() ? fallbackProjectName() : baseName;
    }

    QString AudioExporterPrivate::projectDirectory() const {
        const auto projectDocumentContext = windowHandle->projectDocumentContext();
        Q_ASSERT(projectDocumentContext);

        if (projectDocumentContext->fileLocker()) {
            const QFileInfo fileInfo(projectDocumentContext->fileLocker()->path());
            if (!fileInfo.filePath().isEmpty() && !fileInfo.dir().isRelative()) {
                return fileInfo.dir().path();
            }
        }
        return documentsDirectory();
    }

    QList<dspx::Track *> AudioExporterPrivate::tracks() const {
        const auto projectDocumentContext = windowHandle->projectDocumentContext();
        Q_ASSERT(projectDocumentContext);
        const auto document = projectDocumentContext->document();
        Q_ASSERT(document);
        const auto model = document->model();
        Q_ASSERT(model);
        return model->tracks()->items();
    }

    QString AudioExporterPrivate::trackName(int trackIndex) const {
        const auto trackList = tracks();
        Q_ASSERT(trackIndex >= 0 && trackIndex < trackList.size());
        return trackList.at(trackIndex)->name();
    }

    talcs::DspxProjectContext *AudioExporterPrivate::projectContext() const {
        const auto audioContext = ProjectAudioContext::of(windowHandle);
        Q_ASSERT(audioContext);
        auto context = ProjectAudioContextPrivate::of(audioContext)->projectContext.get();
        Q_ASSERT(context);
        return context;
    }

    QPair<int, int> AudioExporterPrivate::calculateRange() const {
        const auto projectDocumentContext = windowHandle->projectDocumentContext();
        Q_ASSERT(projectDocumentContext);
        const auto document = projectDocumentContext->document();
        Q_ASSERT(document);
        const auto model = document->model();
        Q_ASSERT(model);

        auto timeline = model->timeline();
        if (config.timeRange() == AudioExporterConfig::TR_LoopSection && timeline->isLoopEnabled() && timeline->loopLength() > 0) {
            return {timeline->loopStart(), timeline->loopLength()};
        }

        auto projectTimeline = windowHandle->projectTimeline();
        Q_ASSERT(projectTimeline);
        // TODO change `rangeHint` to the real project length (类似ProjectWindowInterface::boundTimelineRangeHint，但是只考虑最远的剪辑)
        return {0, projectTimeline->rangeHint()};
    }

    QList<int> AudioExporterPrivate::sourceIndexes(bool *ok) const {
        if (ok) {
            *ok = true;
        }

        const auto trackList = tracks();
        QList<int> indexes;
        switch (config.sourceOption()) {
            case AudioExporterConfig::SO_All:
                indexes.reserve(trackList.size());
                for (int i = 0; i < trackList.size(); ++i) {
                    indexes.append(i);
                }
                break;
            case AudioExporterConfig::SO_Selected: {
                const auto selectionModel = windowHandle->projectDocumentContext()->document()->selectionModel();
                Q_ASSERT(selectionModel);
                const auto selectedTracks = selectionModel->trackSelectionModel()->selectedItems();
                indexes.reserve(selectedTracks.size());
                for (auto track : selectedTracks) {
                    const auto index = trackList.indexOf(track);
                    if (index < 0) {
                        if (ok) {
                            *ok = false;
                        }
                        return {};
                    }
                    indexes.append(index);
                }
                std::sort(indexes.begin(), indexes.end());
                break;
            }
            case AudioExporterConfig::SO_Custom:
                indexes = config.source();
                for (const auto index : indexes) {
                    if (index < 0 || index >= trackList.size()) {
                        if (ok) {
                            *ok = false;
                        }
                        return {};
                    }
                }
                break;
        }
        return indexes;
    }

    QList<talcs::DspxTrackContext *> AudioExporterPrivate::sourceTrackContexts(const QList<int> &sourceIndexes) const {
        const auto contextTracks = projectContext()->tracks();
        QList<talcs::DspxTrackContext *> result;
        result.reserve(sourceIndexes.size());
        for (const auto index : sourceIndexes) {
            Q_ASSERT(index >= 0 && index < contextTracks.size());
            result.append(contextTracks.at(index));
        }
        return result;
    }

    bool AudioExporterPrivate::resolveTemplate(QString &templateString) const {
        return resolveTemplate(templateString, {}, -1);
    }

    bool AudioExporterPrivate::resolveTemplate(QString &templateString, const QString &trackName, int trackIndex) const {
        static const QRegularExpression re(QStringLiteral(R"(\$\{(.*?)\})"));
        bool allTemplatesMatch = true;
        qsizetype pos = 0;
        QString result;
        auto matchIt = re.globalMatch(templateString);
        while (matchIt.hasNext()) {
            const auto match = matchIt.next();
            const auto templateName = match.captured(1);
            auto replacedText = match.captured(0);
            if (templateName == QStringLiteral("projectName")) {
                replacedText = projectName();
            } else if (templateName == QStringLiteral("$")) {
                replacedText = QStringLiteral("$");
            } else if (trackIndex != -1) {
                if (templateName == QStringLiteral("trackName")) {
                    replacedText = trackName;
                } else if (templateName == QStringLiteral("trackIndex")) {
                    replacedText = QString::number(trackIndex + 1);
                } else {
                    allTemplatesMatch = false;
                }
            } else {
                allTemplatesMatch = false;
            }
            result += templateString.mid(pos, match.capturedStart(0) - pos);
            result += replacedText;
            pos = match.capturedEnd(0);
        }
        result += templateString.mid(pos);
        templateString = result;
        return allTemplatesMatch;
    }

    int AudioExporterPrivate::calculateFormat() const {
        int format = 0;
        switch (config.fileType()) {
            case AudioExporterConfig::FT_Wav:
                format |= talcs::AudioFormatIO::WAV;
                switch (config.formatOption()) {
                    case 0:
                        format |= talcs::AudioFormatIO::FLOAT;
                        return format;
                    case 1:
                        format |= talcs::AudioFormatIO::PCM_24;
                        return format;
                    case 2:
                        format |= talcs::AudioFormatIO::PCM_16;
                        return format;
                    case 3:
                        format |= talcs::AudioFormatIO::PCM_U8;
                        return format;
                }
                break;
            case AudioExporterConfig::FT_Flac:
                format |= talcs::AudioFormatIO::FLAC;
                switch (config.formatOption()) {
                    case 0:
                        format |= talcs::AudioFormatIO::PCM_24;
                        return format;
                    case 1:
                        format |= talcs::AudioFormatIO::PCM_16;
                        return format;
                    case 2:
                        format |= talcs::AudioFormatIO::PCM_S8;
                        return format;
                }
                break;
            case AudioExporterConfig::FT_OggVorbis:
                format |= static_cast<int>(talcs::AudioFormatIO::OGG) | static_cast<int>(talcs::AudioFormatIO::VORBIS);
                return format;
            case AudioExporterConfig::FT_Mp3:
                format |= static_cast<int>(talcs::AudioFormatIO::MPEG) | static_cast<int>(talcs::AudioFormatIO::MPEG_LAYER_III);
                return format;
        }
        return 0;
    }

    void AudioExporterPrivate::refreshPreflight() {
        Q_Q(AudioExporter);

        auto oldWarnings = preflightWarnings;
        auto oldFileList = fileList;

        preflightWarnings = {};
        fileList.clear();

        if (config.fileType() == AudioExporterConfig::FT_Mp3 || config.fileType() == AudioExporterConfig::FT_OggVorbis) {
            preflightWarnings |= AudioExporter::PW_LossyFormat;
        }

        bool sourcesOk = true;
        const auto indexes = sourceIndexes(&sourcesOk);
        if (!sourcesOk || indexes.isEmpty()) {
            preflightWarnings |= AudioExporter::PW_NoFile;
        }

        if (indexes.isEmpty()) {
            if (oldWarnings != preflightWarnings) {
                Q_EMIT q->preflightWarningsChanged();
            }
            if (oldFileList != fileList) {
                Q_EMIT q->fileListChanged();
            }
            return;
        }

        const auto directory = QDir(projectDirectory()).absoluteFilePath(config.fileDirectory());
        if (config.mixingOption() == AudioExporterConfig::MO_Mixed) {
            auto fileName = config.fileName();
            if (!resolveTemplate(fileName)) {
                preflightWarnings |= AudioExporter::PW_UnrecognizedTemplate;
            }
            const auto fileInfo = QFileInfo(QDir(directory).absoluteFilePath(fileName));
            if (fileInfo.exists()) {
                preflightWarnings |= AudioExporter::PW_WillOverwrite;
            }
            fileList.append(fileInfo.absoluteFilePath());
        } else {
            QSet<QString> fileSet;
            for (const auto index : indexes) {
                auto fileName = config.fileName();
                if (!resolveTemplate(fileName, trackName(index), index)) {
                    preflightWarnings |= AudioExporter::PW_UnrecognizedTemplate;
                }
                const auto fileInfo = QFileInfo(QDir(directory).absoluteFilePath(fileName));
                const auto filePath = fileInfo.absoluteFilePath();
                if (fileInfo.exists()) {
                    preflightWarnings |= AudioExporter::PW_WillOverwrite;
                }
                if (fileSet.contains(filePath)) {
                    preflightWarnings |= AudioExporter::PW_DuplicatedFile;
                }
                fileSet.insert(filePath);
                fileList.append(filePath);
            }
        }

        if (oldWarnings != preflightWarnings) {
            Q_EMIT q->preflightWarningsChanged();
        }
        if (oldFileList != fileList) {
            Q_EMIT q->fileListChanged();
        }
    }

    void AudioExporterPrivate::setError(AudioExporter::Error newError, const QString &message) {
        Q_Q(AudioExporter);
        QString newErrorString = message;
        if (newError == AudioExporter::NoError) {
            newErrorString.clear();
        } else if (newErrorString.isEmpty()) {
            switch (newError) {
                case AudioExporter::InvalidConfig:
                    newErrorString = AudioExporter::tr("Invalid audio export configuration.");
                    break;
                case AudioExporter::InvalidSource:
                    newErrorString = AudioExporter::tr("Invalid audio export source.");
                    break;
                case AudioExporter::CannotOpenFile:
                    newErrorString = AudioExporter::tr("Cannot open file for writing.");
                    break;
                case AudioExporter::UnsupportedFormat:
                    newErrorString = AudioExporter::tr("Audio format is not supported.");
                    break;
                case AudioExporter::CannotStartExport:
                    newErrorString = AudioExporter::tr("Cannot start audio exporting.");
                    break;
                case AudioExporter::ExporterFailed:
                    newErrorString = AudioExporter::tr("Audio exporting failed.");
                    break;
                case AudioExporter::RenameTemporaryFileFailed:
                    newErrorString = AudioExporter::tr("Cannot rename temporary files to target files.");
                    break;
                case AudioExporter::Cancelled:
                    newErrorString = AudioExporter::tr("Audio exporting was cancelled.");
                    break;
                case AudioExporter::UnknownError:
                    newErrorString = AudioExporter::tr("Unknown error.");
                    break;
                case AudioExporter::NoError:
                    break;
            }
        }

        if (error == newError && errorString == newErrorString) {
            return;
        }
        error = newError;
        errorString = newErrorString;
        Q_EMIT q->errorChanged();
    }

    void AudioExporterPrivate::clearError() {
        setError(AudioExporter::NoError);
    }

    void AudioExporterPrivate::setRunning(bool newRunning) {
        Q_Q(AudioExporter);
        if (running == newRunning) {
            return;
        }
        running = newRunning;
        Q_EMIT q->runningChanged();
    }

    AudioExporter::AudioExporter(Core::ProjectWindowInterface *windowHandle, QObject *parent)
        : QObject(parent), d_ptr(new AudioExporterPrivate) {
        Q_ASSERT(windowHandle);
        Q_D(AudioExporter);
        d->q_ptr = this;
        d->windowHandle = windowHandle;
    }

    AudioExporter::~AudioExporter() = default;

    Core::ProjectWindowInterface *AudioExporter::windowHandle() const {
        Q_D(const AudioExporter);
        return d->windowHandle;
    }

    void AudioExporter::registerListener(AudioExporterListener *listener) {
        if (listener && !s_listeners.contains(listener)) {
            s_listeners.append(listener);
        }
    }

    AudioExporterConfig AudioExporter::config() const {
        Q_D(const AudioExporter);
        return d->config;
    }

    void AudioExporter::setConfig(const AudioExporterConfig &config) {
        Q_D(AudioExporter);
        if (d->configInitialized && d->config.toJsonObject() == config.toJsonObject()) {
            return;
        }
        d->config = config;
        d->configInitialized = true;
        Q_EMIT configChanged();
        d->refreshPreflight();
    }

    AudioExporter::PreflightWarnings AudioExporter::preflightWarnings() const {
        Q_D(const AudioExporter);
        return d->preflightWarnings;
    }

    QStringList AudioExporter::preflightWarningTexts(PreflightWarnings warnings) {
        QStringList list;
        if (warnings & PW_NoFile) {
            list.append(tr("No file will be exported. Please check if any source is selected."));
        }
        if (warnings & PW_DuplicatedFile) {
            list.append(tr("The files to be exported contain files with duplicate names. Please check if the file name template is unique for each source."));
        }
        if (warnings & PW_WillOverwrite) {
            list.append(tr("The files to be exported contain files with the same name as existing files. If continue, the existing files will be overwritten."));
        }
        if (warnings & PW_UnrecognizedTemplate) {
            list.append(tr("Unrecognized file name template. Please check the syntax of the file name template."));
        }
        if (warnings & PW_LossyFormat) {
            list.append(tr("The currently selected file type is a lossy format. To avoid loss of sound quality, please use WAV or FLAC format."));
        }
        return list;
    }

    QStringList AudioExporter::fileList() const {
        Q_D(const AudioExporter);
        return d->fileList;
    }

    QStringList AudioExporter::dryRun() const {
        return fileList();
    }

    AudioExporter::Error AudioExporter::error() const {
        Q_D(const AudioExporter);
        return d->error;
    }

    QString AudioExporter::errorString() const {
        Q_D(const AudioExporter);
        return d->errorString;
    }

    bool AudioExporter::isRunning() const {
        Q_D(const AudioExporter);
        return d->running;
    }

    AudioExporter::Result AudioExporter::exec() {
        Q_D(AudioExporter);
        Q_ASSERT(d->windowHandle);
        Q_ASSERT(ProjectAudioContext::of(d->windowHandle));

        if (d->running) {
            d->setError(UnknownError, tr("Audio exporting is already running."));
            return R_Fail;
        }

        d->setRunning(true);
        const auto runningGuard = makeScopeGuard([d] {
            d->setRunning(false);
        });

        d->refreshPreflight();
        d->clearError();
        d->temporaryFileList.clear();

        const auto format = d->calculateFormat();
        if (format == 0) {
            d->setError(InvalidConfig);
            return R_Fail;
        }

        bool sourcesOk = true;
        const auto sourceIndexes = d->sourceIndexes(&sourcesOk);
        if (!sourcesOk || sourceIndexes.isEmpty() || d->fileList.isEmpty()) {
            d->setError(InvalidSource, tr("No file will be exported. Please check if any source is selected."));
            return R_Fail;
        }

        const auto projectContext = d->projectContext();
        QHash<QString, QString> temporaryFiles;

        {
            QObject fileOwner;
            std::unique_ptr<talcs::AudioFormatIO[]> ioList(new talcs::AudioFormatIO[d->fileList.size()]);
            const auto uuid = QString::fromLatin1(QByteArray::fromHex(QUuid::createUuid().toByteArray(QUuid::Id128))
                                                      .toBase64(QByteArray::Base64UrlEncoding)
                                                      .mid(0, 8));

            for (int i = 0; i < d->fileList.size(); ++i) {
                auto fileName = d->fileList.at(i);
                if (Internal::AudioPreference::audioExporterUseTemporaryFile()) {
                    auto temporaryFileName = QFileInfo(fileName).dir().filePath(QStringLiteral(".ds$") + uuid + QFileInfo(fileName).fileName() + QStringLiteral(".exporting"));
                    temporaryFiles.insert(fileName, temporaryFileName);
                    d->temporaryFileList.append(temporaryFileName);
                    fileName = temporaryFileName;
                } else {
                    d->temporaryFileList.append(fileName);
                }

                auto file = new QFile(fileName, &fileOwner);
                if (!file->open(QIODevice::WriteOnly)) {
                    d->setError(CannotOpenFile, tr("Cannot open file for writing: %1").arg(fileName));
                    return R_Fail;
                }

                auto &io = ioList[i];
                io.setStream(file);
                io.setSampleRate(d->config.formatSampleRate());
                io.setChannelCount(d->config.formatMono() ? 1 : 2);
                io.setFormat(format);
                if (!io.open(talcs::AbstractAudioFormatIO::Write)) {
                    d->setError(UnsupportedFormat, tr("Format not supported: %1").arg(io.errorString()));
                    return R_Fail;
                }
                io.setCompressionLevel(0.01 * (100 - d->config.formatQuality()));
            }

            talcs::DspxProjectAudioExporter exporter(projectContext);
            d->currentExporter = &exporter;
            const auto exporterGuard = makeScopeGuard([d] {
                d->currentExporter = nullptr;
            });

            exporter.setMonoChannel(d->config.formatMono());
            exporter.setThruMaster(d->config.mixingOption() == AudioExporterConfig::MO_SeparatedThruMaster);
            exporter.setClippingCheckEnabled(Internal::AudioPreference::audioExporterClippingCheckEnabled());
            exporter.setMuteSoloEnabled(d->config.isMuteSoloEnabled());
            const auto [rangeStart, rangeLength] = d->calculateRange();
            exporter.setRange(rangeStart, rangeLength);

            const auto sourceTracks = d->sourceTrackContexts(sourceIndexes);
            if (d->config.mixingOption() == AudioExporterConfig::MO_Mixed) {
                exporter.setMixedTask(sourceTracks, &ioList[0]);
            } else {
                Q_ASSERT(sourceTracks.size() == d->fileList.size());
                for (int i = 0; i < sourceTracks.size(); ++i) {
                    exporter.addSeparatedTask(sourceTracks.at(i), &ioList[i]);
                }
            }

            QHash<talcs::DspxTrackContext *, int> sourceIndexMap;
            sourceIndexMap.insert(nullptr, -1);
            for (int i = 0; i < sourceTracks.size(); ++i) {
                sourceIndexMap.insert(sourceTracks.at(i), i);
            }

            projectContext->transport()->pause();

            QList<AudioExporterListener *> listenerToCallFinishList;
            const auto callFinishGuard = makeScopeGuard([&] {
                for (const auto listener : listenerToCallFinishList) {
                    listener->willFinishCallback(this);
                }
            });

            const auto currentBufferSize = projectContext->preMixer()->bufferSize();
            const auto currentSampleRate = projectContext->preMixer()->sampleRate();
            const auto reopenMixerGuard = makeScopeGuard([&] {
                if (!projectContext->preMixer()->open(currentBufferSize, currentSampleRate)) {
                    addRuntimeWarning(tr("Cannot reopen audio after exported"));
                }
            });

            if (!projectContext->preMixer()->open(1024, d->config.formatSampleRate())) {
                d->setError(CannotStartExport);
                return R_Fail;
            }

            for (const auto listener : s_listeners) {
                if (!listener->willStartCallback(this)) {
                    if (d->error == NoError) {
                        d->setError(ExporterFailed, tr("Audio export was rejected by a listener."));
                    }
                    return R_Fail;
                }
                listenerToCallFinishList.prepend(listener);
            }

            connect(&exporter, &talcs::DspxProjectAudioExporter::progressChanged, this, [sourceIndexMap, this](double progressRatio, talcs::DspxTrackContext *track) {
                Q_EMIT progressChanged(progressRatio, sourceIndexMap.value(track));
            });
            connect(&exporter, &talcs::DspxProjectAudioExporter::clippingDetected, this, [sourceIndexMap, this](talcs::DspxTrackContext *track) {
                Q_EMIT clippingDetected(sourceIndexMap.value(track));
            });

            const auto result = exporter.exec();
            if (result == talcs::DspxProjectAudioExporter::Fail) {
                if (d->error == NoError) {
                    d->setError(ExporterFailed, tr("Internal Error"));
                }
                return R_Fail;
            }
            if (result == talcs::DspxProjectAudioExporter::Interrupted) {
                d->setError(Cancelled);
                return R_Abort;
            }
        }

        d->temporaryFileList.clear();

        if (Internal::AudioPreference::audioExporterUseTemporaryFile()) {
            auto temporaryFileErrorString = tr("Cannot rename temporary files to target files");
            bool failed = false;
            for (const auto &fileName : temporaryFiles.keys()) {
                QFile::remove(fileName);
                QFile temporaryFile(temporaryFiles.value(fileName));
                if (!temporaryFile.rename(fileName)) {
                    temporaryFileErrorString += QStringLiteral("\n") + fileName;
                    failed = true;
                    d->temporaryFileList.append(temporaryFile.fileName());
                }
            }
            if (failed) {
                d->setError(RenameTemporaryFileFailed, temporaryFileErrorString);
                return R_Fail;
            }
        }

        return R_Ok;
    }

    void AudioExporter::cleanUp() {
        Q_D(AudioExporter);
        for (const auto &fileName : d->temporaryFileList) {
            QFile::remove(fileName);
        }
        d->temporaryFileList.clear();
    }

    void AudioExporter::cancel(bool isFail, const QString &message) {
        Q_D(AudioExporter);
        if (!d->currentExporter) {
            return;
        }
        if (isFail) {
            d->setError(UnknownError, message.isEmpty() ? tr("Unknown error.") : message);
        }
        d->currentExporter->interrupt(isFail);
    }

    void AudioExporter::addRuntimeWarning(const QString &message, int sourceIndex) {
        Q_EMIT runtimeWarningAdded(message, sourceIndex);
    }

}

#include "moc_AudioExporter.cpp"
