// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "AudioClipImporter.h"
#include "AudioClipImporter_p.h"

#include <cmath>

#include <QDir>
#include <QFileInfo>
#include <QLoggingCategory>

#include <CoreApi/filelocker.h>

#include <SVSCraftCore/MusicTime.h>
#include <SVSCraftCore/MusicTimeline.h>

#include <TalcsFormat/FormatEntry.h>
#include <TalcsFormat/FormatManager.h>

#include <dspxmodelORM/ClipSequence.h>
#include <dspxmodelORM/Model.h>
#include <dspxmodelORM/Track.h>

#include <coreplugin/DspxDocument.h>
#include <coreplugin/ProjectDocumentContext.h>
#include <coreplugin/ProjectTimeline.h>
#include <coreplugin/ProjectWindowInterface.h>

#include <transactional/TransactionController.h>

#include <audio/GlobalAudioContext.h>
#include <audio/internal/HashHelper.h>
#include <audio/internal/ProjectAudioAddOn.h>

namespace Audio {

    Q_STATIC_LOGGING_CATEGORY(lcAudioClipImporter, "diffscope.audio.audioclipimporter")

    AudioClipImporter::AudioClipImporter(Core::ProjectWindowInterface *windowHandle, QObject *parent)
        : QObject(parent), d_ptr(new AudioClipImporterPrivate) {
        Q_D(AudioClipImporter);
        d->q_ptr = this;
        d->windowHandle = windowHandle;
        d->document = windowHandle ? windowHandle->projectDocumentContext()->document() : nullptr;
    }

    AudioClipImporter::~AudioClipImporter() = default;

    bool AudioClipImporter::openFile(const QString &fileName) {
        Q_D(AudioClipImporter);
        const QPointer<AudioClipImporter> guard(this);
        d->io.reset();
        d->fileName.clear();
        if (!d->windowHandle || !d->document || !QFileInfo(fileName).isFile()) {
            return false;
        }

        const auto manager = GlobalAudioContext::formatManager();
        auto entries = manager->entries();
        if (auto hintedEntry = manager->hintFromExtension(QFileInfo(fileName).suffix().toLower())) {
            entries.removeOne(hintedEntry);
            entries.prepend(hintedEntry);
        }
        for (auto entry : entries) {
            QVariant userData;
            std::unique_ptr<talcs::AbstractAudioFormatIO> io(entry->getFormatOpen(fileName, userData, d->windowHandle->invisibleCentralWidget()));
            if (!guard || !d->windowHandle || !d->document || d->windowHandle->projectDocumentContext()->document() != d->document) {
                return false;
            }
            if (io) {
                return setFile(fileName, io.release(), userData, QString::fromLatin1(entry->metaObject()->className()));
            }
        }
        qCWarning(lcAudioClipImporter) << "Failed to open audio file" << fileName;
        return false;
    }

    bool AudioClipImporter::setFile(const QString &fileName, talcs::AbstractAudioFormatIO *io, const QVariant &userData, const QString &entryClassName) {
        Q_D(AudioClipImporter);
        d->io.reset(io);
        d->fileName.clear();
        if (!d->windowHandle || !d->document || !d->io || !d->io->open(talcs::AbstractAudioFormatIO::Read)) {
            d->io.reset();
            return false;
        }
        const auto sampleRate = d->io->sampleRate();
        const auto length = d->io->length();
        if (!std::isfinite(sampleRate) || sampleRate <= 0 || length < 0 || d->io->channelCount() <= 0) {
            qCWarning(lcAudioClipImporter) << "Invalid audio metadata" << fileName;
            d->io.reset();
            return false;
        }
        d->path = createAudioPath(d->windowHandle, fileName, userData, entryClassName);
        if (d->path.digest.isEmpty()) {
            qCWarning(lcAudioClipImporter) << "Failed to calculate audio file digest" << fileName;
            d->io.reset();
            return false;
        }
        d->durationMsec = static_cast<double>(length) * 1000.0 / sampleRate;
        d->fileName = QFileInfo(fileName).absoluteFilePath();
        return true;
    }

    talcs::AbstractAudioFormatIO *AudioClipImporter::audioFormatIO() const {
        Q_D(const AudioClipImporter);
        return d->io.get();
    }

    QString AudioClipImporter::fileName() const {
        Q_D(const AudioClipImporter);
        return d->fileName;
    }

    dspx::AudioClip *AudioClipImporter::insertClip(dspx::Track *track, int position, const QString &name) {
        Q_D(AudioClipImporter);
        if (!d->windowHandle || !d->document || !d->windowHandle->projectTimeline() || d->windowHandle->projectDocumentContext()->document() != d->document || !d->io || !track || track->model() != d->document->model() || !track->trackList() || !d->document->transactionController()->isExclusiveToTransaction()) {
            return nullptr;
        }
        if (!(d->io->openMode() & talcs::AbstractAudioFormatIO::Read) && !d->io->open(talcs::AbstractAudioFormatIO::Read)) {
            return nullptr;
        }
        d->io->seek(0);
        position = qMax(0, position);
        const auto timeline = d->windowHandle->projectTimeline()->musicTimeline();
        const auto positionMsec = timeline->create(0, 0, position).millisecond();
        const auto endPosition = timeline->create(positionMsec + d->durationMsec).totalTick();
        const auto model = d->document->model();
        updateAudioPathLocation(d->windowHandle, &d->path, d->fileName);
        auto clip = model->createAudioClip();
        clip->setName(name);
        clip->setPath(d->path);
        clip->setClipStart(0);
        clip->setClipLength(qMax(1, endPosition - position));
        clip->setPosition(position);
        clip->setGain(1);

        auto projectAudioAddOn = Internal::ProjectAudioAddOn::of(d->windowHandle);
        if (projectAudioAddOn) {
            projectAudioAddOn->addAudioClipCache(clip, d->io.release());
        }
        if (!track->clips()->insertItem(clip)) {
            if (projectAudioAddOn) {
                d->io.reset(projectAudioAddOn->takeAudioClipCache(clip));
            }
            model->destroyItem(clip);
            return nullptr;
        }
        d->io.reset();
        d->fileName.clear();
        return clip;
    }

    dspx::AudioPathInfo AudioClipImporter::createAudioPath(Core::ProjectWindowInterface *windowHandle, const QString &fileName, const QVariant &userData, const QString &entryClassName) {
        dspx::AudioPathInfo path;
        updateAudioPathLocation(windowHandle, &path, fileName);
        path.formatEntryClassName = entryClassName;
        path.userData = userData;
        path.digest = Internal::HashHelper::digest(QFileInfo(fileName).absoluteFilePath());
        return path;
    }

    void AudioClipImporter::updateAudioPathLocation(Core::ProjectWindowInterface *windowHandle, dspx::AudioPathInfo *path, const QString &fileName) {
        if (!path) {
            return;
        }
        const QFileInfo fileInfo(fileName);
        path->absoluteDir = fileInfo.absolutePath();
        path->relativeDir = {};
        path->fileName = fileInfo.fileName();

        const auto context = windowHandle ? windowHandle->projectDocumentContext() : nullptr;
        const auto fileLocker = context ? context->fileLocker() : nullptr;
        const auto projectPath = fileLocker ? fileLocker->path() : QString();
        if (!projectPath.isEmpty()) {
            const QDir projectDir(QFileInfo(projectPath).absolutePath());
            const auto relativePath = projectDir.relativeFilePath(fileInfo.absoluteFilePath());
            if (relativePath == "." || (!relativePath.startsWith("../") && relativePath != ".." && !QDir::isAbsolutePath(relativePath))) {
                path->relativeDir = projectDir.relativeFilePath(fileInfo.absolutePath());
            }
        }
    }

}

#include "moc_AudioClipImporter.cpp"
