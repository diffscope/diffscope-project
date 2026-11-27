// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "EmbeddedAudio.h"
#include "EmbeddedAudio_p.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonObject>
#include <QLoggingCategory>
#include <QPointer>
#include <QPromise>
#include <QSaveFile>
#include <QScopeGuard>
#include <QTemporaryFile>
#include <QtConcurrentRun>

#include <CoreApi/filelocker.h>

#include <opendspx/audioclip.h>
#include <opendspx/model.h>

#include <TalcsFormat/AbstractAudioFormatIO.h>
#include <TalcsFormat/AudioFormatIO.h>
#include <TalcsFormat/FormatManager.h>
#include <TalcsFormat/WavpackAudioFormatIO.h>

#include <dspxmodelORM/AudioClip.h>
#include <dspxmodelORM/ClipSequence.h>
#include <dspxmodelORM/Model.h>
#include <dspxmodelORM/Track.h>
#include <dspxmodelORM/TrackList.h>
#include <dspxproject/ClipboardData.h>
#include <dspxproject/ClipboardEditScenario.h>
#include <dspxproject/Document.h>
#include <dspxproject/ProjectDocumentContext.h>
#include <transactional/TransactionController.h>
#include <audio/AudioClipAudioContext.h>
#include <audio/GlobalAudioContext.h>
#include <audio/internal/EmbeddedAudioCodec.h>
#include <audio/internal/EmbeddedAudioPersistenceHook.h>
#include <audio/private/AudioClipAudioContext_p.h>

namespace Audio {

    Q_STATIC_LOGGING_CATEGORY(lcEmbeddedAudio, "diffscope.audio.embeddedaudio")

    using Internal::EmbeddedAudioFile;

    static constexpr auto clipboardNamespace = "org.diffscope.audio";
    static constexpr auto audioDirectoryName = "embeddedAudio";
    static constexpr qsizetype fileBlockSize = 3 * 64 * 1024;

    static bool validKey(const QString &key) {
        return key.size() == 32 && std::ranges::all_of(key, [](QChar c) {
            return (c >= u'0' && c <= u'9') || (c >= u'a' && c <= u'f');
        });
    }

    static QString embeddedKey(const opendspx::ClipRef &clip) {
        if (!clip || clip->type != opendspx::Clip::Type::Audio) {
            return {};
        }
        const auto &clipWorkspace = std::as_const(clip->workspace);
        const auto workspace = clipWorkspace.find("diffscope");
        if (workspace != clipWorkspace.end()) {
            const auto audio = workspace->second.find("audio");
            if (audio != workspace->second.end()) {
                if (const auto *key = audio->second["embeddedKey"].asString()) {
                    return QString::fromStdString(*key);
                }
            }
        }
        return {};
    }

    static void collectKeys(const std::vector<opendspx::ClipRef> &clips, QSet<QString> &keys) {
        for (const auto &clip : clips) {
            if (auto key = embeddedKey(clip); !key.isEmpty()) {
                keys.insert(std::move(key));
            }
        }
    }

    static QSet<QString> modelKeys(const opendspx::Model &model) {
        QSet<QString> keys;
        for (const auto &track : model.content.tracks) {
            collectKeys(track.clips, keys);
        }
        return keys;
    }

    static QSet<QString> clipboardKeys(const DspxProject::ClipboardData &data) {
        QSet<QString> keys;
        if (data.type() == DspxProject::ClipboardData::Clip) {
            for (const auto &clips : data.clips()) {
                collectKeys(clips, keys);
            }
        } else if (data.type() == DspxProject::ClipboardData::Track) {
            for (const auto &track : data.tracks()) {
                collectKeys(track.clips, keys);
            }
        }
        return keys;
    }

    static std::optional<std::string> encodeBase64(const EmbeddedAudioFile &audio) {
        QFile file(audio.path);
        if (!file.open(QIODevice::ReadOnly)) {
            return {};
        }
        std::string encoded;
        const auto size = file.size();
        if (size <= 0 || quint64(size / 3 + (size % 3 != 0)) > encoded.max_size() / 4) {
            return {};
        }
        encoded.reserve((size / 3 + (size % 3 != 0)) * 4);
        for (qint64 remaining = size; remaining > 0;) {
            const auto length = std::min<qint64>(fileBlockSize, remaining);
            const auto block = file.read(length);
            if (block.size() != length) {
                return {};
            }
            const auto base64 = block.toBase64();
            encoded.append(base64.constData(), base64.size());
            remaining -= length;
        }
        return encoded;
    }

    static bool copyFile(QFile &source, QFileDevice &destination) {
        while (!source.atEnd()) {
            const auto data = source.read(fileBlockSize);
            if (data.isEmpty() || destination.write(data) != data.size()) {
                return false;
            }
        }
        return source.error() == QFileDevice::NoError;
    }

    struct ImportedAudio {
        QSharedPointer<const EmbeddedAudioFile> file;
        bool storageError = false;
    };

    static ImportedAudio importAudio(const QByteArray &magic, const std::function<bool(QTemporaryFile &)> &write) {
        EmbeddedAudio::Compression compression;
        QString suffix;
        if (magic.startsWith("wvpk")) {
            compression = EmbeddedAudio::Compression::Lossless;
            suffix = QStringLiteral(".wv");
        } else if (magic.startsWith("OggS")) {
            compression = EmbeddedAudio::Compression::Lossy;
            suffix = QStringLiteral(".ogg");
        } else {
            return {};
        }
        QTemporaryFile file(QDir::temp().filePath(QStringLiteral("diffscope-XXXXXX") + suffix));
        if (!file.open()) {
            return {.storageError = true};
        }
        if (!write(file)) {
            return {.storageError = file.error() != QFileDevice::NoError};
        }
        if (!file.flush()) {
            return {.storageError = true};
        }
        file.close();
        const std::unique_ptr<talcs::AbstractAudioFormatIO> input(GlobalAudioContext::formatManager()->getFormatLoad(file.fileName()));
        if (!input || (!input->openMode() && !input->open(talcs::AbstractAudioFormatIO::Read))
            || input->channelCount() <= 0 || input->sampleRate() <= 0 || input->length() <= 0) {
            return {};
        }
        if (compression == EmbeddedAudio::Compression::Lossless) {
            const auto *wavPack = dynamic_cast<const talcs::WavpackAudioFormatIO *>(input.get());
            if (!wavPack || !std::isnan(wavPack->bitRate())) {
                return {};
            }
        } else {
            const auto *vorbis = dynamic_cast<const talcs::AudioFormatIO *>(input.get());
            if (!vorbis || vorbis->subtype() != talcs::AudioFormatIO::VORBIS) {
                return {};
            }
        }
        return {QSharedPointer<EmbeddedAudioFile>::create(file, compression)};
    }

    static ImportedAudio decodeBase64(std::string_view encoded) {
        if (encoded.size() < 8 || encoded.size() % 4 != 0) {
            return {};
        }
        const auto magic = QByteArray::fromBase64Encoding(QByteArray::fromRawData(encoded.data(), 8), QByteArray::AbortOnBase64DecodingErrors);
        if (!magic) {
            return {};
        }
        return importAudio(magic.decoded, [&](QTemporaryFile &file) {
            constexpr std::size_t blockSize = 256 * 1024;
            while (!encoded.empty()) {
                const auto length = std::min(encoded.size(), blockSize);
                const auto block = QByteArray::fromRawData(encoded.data(), length);
                if ((length < encoded.size() && block.contains('='))) {
                    return false;
                }
                const auto decoded = QByteArray::fromBase64Encoding(block, QByteArray::AbortOnBase64DecodingErrors);
                if (!decoded || file.write(decoded.decoded) != decoded.decoded.size()) {
                    return false;
                }
                encoded.remove_prefix(length);
            }
            return true;
        });
    }

    static ImportedAudio readJournal(const QString &directory, const QString &key) {
        const auto audioDirectory = QDir(directory).filePath(QString::fromLatin1(audioDirectoryName));
        const auto filePath = QDir(audioDirectory).filePath(key);
        QFile file(filePath);
        if (QFileInfo(audioDirectory).isSymLink() || QFileInfo(filePath).isSymLink() || !file.open(QIODevice::ReadOnly)) {
            return {};
        }
        return importAudio(file.peek(4), [&](QTemporaryFile &output) { return copyFile(file, output); });
    }

    EmbeddedAudio::EmbeddedAudio(DspxProject::ProjectDocumentContext *context)
        : QObject(context), d_ptr(new EmbeddedAudioPrivate{.q_ptr = this, .context = context}) {
    }

    EmbeddedAudio::~EmbeddedAudio() = default;

    EmbeddedAudio *EmbeddedAudio::of(DspxProject::ProjectDocumentContext *context) {
        if (!context) {
            return nullptr;
        }
        auto *audio = context->findChild<EmbeddedAudio *>(Qt::FindDirectChildrenOnly);
        if (!audio) {
            audio = new EmbeddedAudio(context);
        }
        audio->d_func()->bindDocument();
        return audio;
    }

    static QHash<QString, qsizetype> liveReferences(const DspxProject::Document &document) {
        QHash<QString, qsizetype> references;
        for (auto *track : document.model()->tracks()->items()) {
            for (auto *clip : track->clips()->asRange()) {
                if (auto *audio = qobject_cast<dspx::AudioClip *>(clip)) {
                    const auto key = audio->path().embeddedKey;
                    if (!key.isEmpty()) {
                        ++references[key];
                    }
                }
            }
        }
        return references;
    }

    void EmbeddedAudioPrivate::bindDocument() {
        Q_Q(EmbeddedAudio);
        if (document || !context->document()) {
            return;
        }
        document = context->document();
        const auto recordSavedKeys = [this] {
            const auto references = liveReferences(*document);
            savedKeys = QSet<QString>(references.keyBegin(), references.keyEnd());
        };
        if (context->fileLocker() && !context->fileLocker()->path().isEmpty()) {
            recordSavedKeys();
        }
        QObject::connect(context, &DspxProject::ProjectDocumentContext::saved, q, recordSavedKeys);
    }

    bool EmbeddedAudioPrivate::writeJournal(const QString &key, const EmbeddedAudioFile &audio) const {
        const auto directory = context->documentLogDirectory();
        if (directory.isEmpty()) {
            return true;
        }
        const auto audioPath = QDir(directory).filePath(QString::fromLatin1(audioDirectoryName));
        if (QFileInfo(audioPath).isSymLink() || !QDir().mkpath(audioPath)) {
            return false;
        }
        const auto filePath = QDir(audioPath).filePath(key);
        if (QFileInfo(filePath).isSymLink()) {
            return false;
        }
        QFile input(audio.path);
        QSaveFile output(filePath);
        output.setDirectWriteFallback(false);
        return input.open(QIODevice::ReadOnly) && output.open(QIODevice::WriteOnly) && copyFile(input, output) && output.commit();
    }

    bool EmbeddedAudioPrivate::publish(const Files &files) {
        Q_Q(EmbeddedAudio);
        for (auto [key, file] : files.asKeyValueRange()) {
            if (!writeJournal(key, *file)) {
                return false;
            }
            auto &entry = entries[key];
            if (entry.file) {
                retiredFiles.append(entry.file);
            }
            entry = {file, ++generation};
            if (document && savedKeys.contains(key)) {
                document->transactionController()->setCleanStep(-1);
            }
            Q_EMIT q->contentChanged(key);
        }
        return true;
    }

    QString EmbeddedAudio::filePath(const QString &key) const {
        Q_D(const EmbeddedAudio);
        const auto file = d->entries.value(key).file;
        return file ? file->path : QString();
    }

    bool EmbeddedAudio::isLossless(const QString &key) const {
        Q_D(const EmbeddedAudio);
        const auto file = d->entries.value(key).file;
        return file && file->compression == Compression::Lossless;
    }

    bool EmbeddedAudio::extractToFile(const QString &key, const QString &filePath) const {
        Q_D(const EmbeddedAudio);
        const auto file = d->entries.value(key).file;
        if (!file || QFileInfo(filePath) == QFileInfo(file->path)) {
            return false;
        }
        QSaveFile output(filePath);
        if (!output.open(QIODevice::WriteOnly)) {
            return false;
        }
        if (file->compression == Compression::Lossless) {
            return Internal::extractWavPack(file->path, output) && output.commit();
        }
        QFile input(file->path);
        return input.open(QIODevice::ReadOnly) && copyFile(input, output) && output.commit();
    }

    QFuture<EmbeddedAudio::Prepared> EmbeddedAudio::prepare(const StreamFormat &format, FrameReader reader, Compression compression) {
        return QtConcurrent::run([format, reader = std::move(reader), compression](QPromise<Prepared> &promise) {
            Prepared result;
            result.m_compression = compression;
            if (format.sampleType == SampleType::Other) {
                qCWarning(lcEmbeddedAudio) << "Embedding audio from the Float32 read stream";
            }
            const auto output = Internal::encodeEmbeddedAudio(format, reader, compression, 0, [&] { return promise.isCanceled(); });
            result.m_key = output.key;
            result.m_file = output.file;
            if (!result.m_file) {
                result.m_error = tr("Could not encode the audio.");
            }
            promise.addResult(std::move(result));
        });
    }

    QFuture<EmbeddedAudio::Prepared> EmbeddedAudio::prepare(const QString &path, const QVariant &userData, const QString &formatEntry, Compression compression) {
        return QtConcurrent::run([path, userData, formatEntry, compression](QPromise<Prepared> &promise) {
            Prepared result;
            result.m_compression = compression;
            const std::unique_ptr<talcs::AbstractAudioFormatIO> input(GlobalAudioContext::formatManager()->getFormatLoad(path, userData, formatEntry));
            if (!input || (!input->openMode() && !input->open(talcs::AbstractAudioFormatIO::Read))) {
                result.m_error = tr("Could not open the source audio.");
                promise.addResult(std::move(result));
                return;
            }
            const auto rate = input->sampleRate();
            if (!std::isfinite(rate) || rate <= 0 || rate > std::numeric_limits<int>::max() || std::trunc(rate) != rate) {
                result.m_error = tr("The source audio sample rate is unsupported.");
                promise.addResult(std::move(result));
                return;
            }
            StreamFormat format{static_cast<int>(rate), input->channelCount(), input->length(), SampleType::Other};
            double pcmScale = 0;
            if (auto *standard = dynamic_cast<talcs::AudioFormatIO *>(input.get())) {
                switch (standard->subtype()) {
                    case talcs::AudioFormatIO::PCM_U8:
                    case talcs::AudioFormatIO::PCM_S8: format.sampleType = SampleType::Int8; break;
                    case talcs::AudioFormatIO::PCM_16: format.sampleType = SampleType::Int16; break;
                    case talcs::AudioFormatIO::PCM_24: format.sampleType = SampleType::Int24; break;
                    case talcs::AudioFormatIO::FLOAT: format.sampleType = SampleType::Float32; break;
                    default: break;
                }
            } else if (dynamic_cast<talcs::WavpackAudioFormatIO *>(input.get())) {
                const auto bits = input->format();
                switch (bits) {
                    case talcs::WavpackAudioFormatIO::Float: format.sampleType = SampleType::Float32; break;
                    case 8: format.sampleType = SampleType::Int8; break;
                    case 16: format.sampleType = SampleType::Int16; break;
                    case 24: format.sampleType = SampleType::Int24; break;
                    default: break;
                }
                if (bits >= 8 && bits <= 24) {
                    pcmScale = std::ldexp(1.0, bits - 1) - 1 + 0.49999;
                }
            }
            if (format.sampleType == SampleType::Other) {
                qCWarning(lcEmbeddedAudio) << "Embedding audio from the Float32 read stream";
            }
            const auto output = Internal::encodeEmbeddedAudio(format, [&](float *frames, qint64 count) {
                return input->read(frames, count);
            }, compression, pcmScale, [&] { return promise.isCanceled(); });
            result.m_key = output.key;
            result.m_file = output.file;
            if (!result.m_file) {
                result.m_error = tr("Could not encode the audio.");
            }
            promise.addResult(std::move(result));
        });
    }

    EmbeddedAudio::Conflict EmbeddedAudio::conflict(const Prepared &prepared) const {
        Q_D(const EmbeddedAudio);
        const auto entry = d->entries.value(prepared.m_key);
        const auto action = !entry.file ? Conflict::Add : entry.file->compression == prepared.m_compression ? Conflict::Reuse
            : prepared.m_compression == Compression::Lossless ? Conflict::Upgrade : Conflict::Downgrade;
        const auto references = d->document ? liveReferences(*d->document).value(prepared.m_key) : 0;
        return {action, prepared.m_key, references, entry.generation};
    }

    EmbeddedAudio::ApplyResult EmbeddedAudio::apply(const QList<Prepared> &prepared, const QList<Conflict> &expected, const QString &transactionName,
                                                     const std::function<bool(const QList<dspx::AudioPathInfo> &)> &edit, bool allowDowngrade) {
        Q_D(EmbeddedAudio);
        d->bindDocument();
        if (!d->document || !edit || prepared.isEmpty() || prepared.size() != expected.size()) {
            return ApplyResult::Stale;
        }
        QList<dspx::AudioPathInfo> paths;
        EmbeddedAudioPrivate::Files files;
        QHash<QString, Compression> compressions;
        for (qsizetype i = 0; i < prepared.size(); ++i) {
            const auto &audio = prepared[i];
            const auto actual = conflict(audio);
            if (!audio.m_file || !validKey(audio.m_key) || audio.m_file->compression != audio.m_compression
                || actual.key != expected[i].key || actual.generation != expected[i].generation || actual.action != expected[i].action
                || (actual.action == Conflict::Downgrade && actual.referenceCount != expected[i].referenceCount)
                || compressions.value(audio.m_key, audio.m_compression) != audio.m_compression) {
                return ApplyResult::Stale;
            }
            if (actual.action == Conflict::Downgrade && !allowDowngrade) {
                return ApplyResult::DowngradeRequired;
            }
            compressions.insert(audio.m_key, audio.m_compression);
            if (actual.action != Conflict::Reuse) {
                files.insert(audio.m_key, audio.m_file);
            }
            paths.append(dspx::AudioPathInfo{.embeddedKey = audio.m_key});
        }
        auto *controller = d->document->transactionController();
        const auto initialStep = controller->currentStep();
        const auto transaction = controller->beginTransaction();
        if (transaction == Core::TransactionController::TransactionId::Invalid) {
            return ApplyResult::Busy;
        }
        auto abort = qScopeGuard([&] { controller->abortTransaction(transaction); });
        if (!edit(paths)) {
            return ApplyResult::NoChange;
        }
        if (!d->publish(files)) {
            return ApplyResult::WriteFailed;
        }
        try {
            if (!controller->commitTransaction(transaction, transactionName)) {
                return ApplyResult::WriteFailed;
            }
        } catch (const std::exception &error) {
            qCWarning(lcEmbeddedAudio) << "Failed to commit the audio clip transaction" << error.what();
            return ApplyResult::WriteFailed;
        }
        abort.dismiss();
        return files.isEmpty() && controller->currentStep() == initialStep ? ApplyResult::NoChange : ApplyResult::Applied;
    }

    void EmbeddedAudio::configureClipboard(DspxProject::ClipboardEditScenario *scenario, const std::function<void()> &reportCopyFailure) {
        const auto self = QPointer(this);
        scenario->setAttachmentHandlers([self, reportCopyFailure](DspxProject::ClipboardData &data) {
            if (!self) {
                return false;
            }
            const auto keys = clipboardKeys(data);
            if (keys.isEmpty()) {
                return true;
            }
            QJsonObject audio;
            for (const auto &key : keys) {
                const auto file = self->d_func()->entries.value(key).file;
                const auto encoded = file ? encodeBase64(*file) : std::nullopt;
                if (!encoded) {
                    reportCopyFailure();
                    return false;
                }
                audio.insert(key, QString::fromLatin1(encoded->data(), encoded->size()));
            }
            auto attachments = data.attachments();
            attachments.insert(QString::fromLatin1(clipboardNamespace), QJsonObject{{QString::fromLatin1(audioDirectoryName), audio}});
            data.setAttachments(attachments);
            return true;
        }, [self](const DspxProject::ClipboardData &data) {
            if (!self) {
                return false;
            }
            const auto keys = clipboardKeys(data);
            if (keys.isEmpty()) {
                return true;
            }
            const auto audio = data.attachments().value(QString::fromLatin1(clipboardNamespace)).toObject().value(QString::fromLatin1(audioDirectoryName)).toObject();
            EmbeddedAudioPrivate::Files files;
            for (const auto &key : keys) {
                if (!validKey(key)) {
                    return false;
                }
                if (!self->d_func()->entries.value(key).file) {
                    const auto encoded = audio.value(key).toString().toLatin1();
                    const auto imported = decodeBase64(std::string_view(encoded.constData(), encoded.size()));
                    if (!imported.file) {
                        return false;
                    }
                    files.insert(key, imported.file);
                }
            }
            return self->d_func()->publish(files);
        });
    }

    void Internal::EmbeddedAudioPersistenceHook::beforeLoadModel(DspxProject::ProjectDocumentContext &context, opendspx::Model &model, const QString &recoverySourceDirectory) {
        auto *store = EmbeddedAudio::of(&context)->d_func();
        store->entries.clear();
        stdc::json::Object::node_type audioNode;
        if (auto workspace = model.content.workspace.find("diffscope"); workspace != model.content.workspace.end()) {
            audioNode = workspace->second.extract("embeddedAudio");
        }
        const auto *audio = audioNode.empty() ? nullptr : audioNode.mapped().asObject();
        EmbeddedAudioPrivate::Files files;
        for (const auto &key : modelKeys(model)) {
            ImportedAudio imported;
            if (validKey(key)) {
                if (!recoverySourceDirectory.isEmpty()) {
                    imported = readJournal(recoverySourceDirectory, key);
                } else if (audio) {
                    if (const auto value = audio->find(key.toStdString()); value != audio->end() && value->second.isString()) {
                        imported = decodeBase64(value->second.toString());
                    }
                }
            }
            if (imported.storageError) {
                throw std::runtime_error(EmbeddedAudio::tr("Could not store embedded audio.").toStdString());
            }
            if (imported.file) {
                files.insert(key, imported.file);
            } else {
                qCWarning(lcEmbeddedAudio) << "Embedded audio is missing or invalid" << key;
            }
        }
        if (!store->publish(files)) {
            throw std::runtime_error(EmbeddedAudio::tr("Could not write embedded audio to the recovery directory.").toStdString());
        }
    }

    void Internal::EmbeddedAudioPersistenceHook::prepareReplacement(DspxProject::ProjectDocumentContext &context, DspxProject::ProjectDocumentContext &candidate) {
        const auto *source = EmbeddedAudio::of(&candidate)->d_func();
        auto *target = EmbeddedAudio::of(&context)->d_func();
        for (const auto &[key, entry] : source->entries.asKeyValueRange()) {
            if (!target->writeJournal(key, *entry.file))
                throw std::runtime_error(EmbeddedAudio::tr("Could not write embedded audio to the recovery directory.").toStdString());
        }
    }

    void Internal::EmbeddedAudioPersistenceHook::finishReplacement(DspxProject::ProjectDocumentContext &context, DspxProject::ProjectDocumentContext &candidate) {
        auto *source = EmbeddedAudio::of(&candidate)->d_func();
        auto *audio = EmbeddedAudio::of(&context);
        auto *target = audio->d_func();
        auto previous = std::exchange(target->entries, {});
        target->entries = std::move(source->entries);
        for (auto &entry : previous)
            target->retiredFiles.append(std::move(entry.file));
        for (auto &entry : target->entries)
            entry.generation = ++target->generation;
        target->savedKeys.clear();
        if (context.fileLocker()) {
            const auto references = liveReferences(*context.document());
            target->savedKeys = QSet<QString>(references.keyBegin(), references.keyEnd());
        }
        for (const auto &key : target->entries.keys())
            Q_EMIT audio->contentChanged(key);
        for (const auto &key : previous.keys()) {
            if (!target->entries.contains(key))
                Q_EMIT audio->contentChanged(key);
        }
    }

    void Internal::EmbeddedAudioPersistenceHook::afterSaveModel(DspxProject::ProjectDocumentContext &context, opendspx::Model &model, const QString &destination) {
        const auto base = QFileInfo(destination).absoluteDir();
        const auto referencePath = context.retainedReferenceFilePath();
        const auto tracks = context.document()->model()->tracks()->items();
        for (qsizetype trackIndex = 0; trackIndex < tracks.size(); ++trackIndex) {
            auto &clips = model.content.tracks.at(trackIndex).clips;
            auto target = clips.begin();
            for (auto *clip : tracks.at(trackIndex)->clips()->asRange()) {
                if (target == clips.end())
                    break;
                auto &serialized = *target++;
                auto *source = qobject_cast<dspx::AudioClip *>(clip);
                if (!source || !serialized || serialized->type != opendspx::Clip::Type::Audio ||
                    !source->path().embeddedKey.isEmpty())
                    continue;
                auto &audio = static_cast<opendspx::AudioClip &>(*serialized);
                auto *audioContext = AudioClipAudioContext::of(source);
                auto resolved = audioContext ? audioContext->realAudioPath() : QString();
                if (!audioContext && !referencePath.isEmpty() && !QFileInfo(QString::fromStdString(audio.path)).isFile())
                    resolved = AudioClipAudioContextPrivate::findReferencedFile(source->path(), referencePath);
                if (resolved.isEmpty() && destination.isEmpty())
                    continue;
                const QFileInfo sourceInfo(resolved.isEmpty() ? QString::fromStdString(audio.path) : resolved);
                if (sourceInfo.fileName().isEmpty())
                    continue;
                audio.path = sourceInfo.absoluteFilePath().toStdString();
                if (!destination.isEmpty()) {
                    auto &metadata = audio.workspace["diffscope"]["audio"];
                    auto properties = metadata.toObject();
                    properties["relativeDir"] = base.relativeFilePath(sourceInfo.absolutePath()).toStdString();
                    metadata = std::move(properties);
                }
            }
        }
        const auto *store = EmbeddedAudio::of(&context)->d_func();
        stdc::json::Object audio;
        for (const auto &key : modelKeys(model)) {
            const auto file = store->entries.value(key).file;
            auto encoded = file ? encodeBase64(*file) : std::nullopt;
            if (!validKey(key) || !encoded) {
                throw std::runtime_error(EmbeddedAudio::tr("Could not save embedded audio with key %1.").arg(key).toStdString());
            }
            audio.emplace(key.toStdString(), std::move(*encoded));
        }
        auto &workspace = model.content.workspace["diffscope"];
        if (audio.empty()) {
            workspace.erase("embeddedAudio");
        } else {
            workspace["embeddedAudio"] = std::move(audio);
        }
    }

}

#include "moc_EmbeddedAudio.cpp"
