// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "AudioFileDropAddOn.h"

#include <memory>
#include <utility>
#include <vector>

#include <QDir>
#include <QFileInfo>
#include <QLoggingCategory>
#include <QMetaObject>
#include <QScopeGuard>
#include <QSet>
#include <QStringList>

#include <CoreApi/objectpool.h>

#include <opendspx/track.h>

#include <SVSCraftCore/SVSCraftNamespace.h>

#include <TalcsFormat/FormatManager.h>

#include <dspxmodelORM/AudioClip.h>
#include <dspxmodelORM/Model.h>
#include <dspxmodelORM/Track.h>
#include <dspxmodelORM/TrackList.h>
#include <dspxmodelSelectionModel/SelectionModel.h>

#include <coreplugin/CoreInterface.h>
#include <coreplugin/DspxDocument.h>
#include <coreplugin/ProjectDocumentContext.h>
#include <coreplugin/ProjectWindowInterface.h>
#include <coreplugin/TrackColorSchema.h>

#include <transactional/TransactionController.h>

#include <audio/AudioClipImporter.h>
#include <audio/GlobalAudioContext.h>

namespace AudioVisualizer::Internal {

    Q_STATIC_LOGGING_CATEGORY(lcAudioFileDropAddOn, "diffscope.audiovisualizer.audiofiledropaddon")

    AudioFileDropAddOnAttachedType::AudioFileDropAddOnAttachedType(QObject *parent) : QObject(parent) {
        if (auto window = qobject_cast<Core::ProjectWindowInterface *>(parent)) {
            connect(window, &Core::ObjectPool::objectAdded, this, [this](const QString &, QObject *object) {
                if (qobject_cast<AudioFileDropAddOn *>(object)) {
                    Q_EMIT addOnChanged();
                }
            });
        }
    }

    AudioFileDropAddOn *AudioFileDropAddOnAttachedType::addOn() const {
        const auto window = qobject_cast<Core::ProjectWindowInterface *>(parent());
        return window ? window->getFirstObject<AudioFileDropAddOn>() : nullptr;
    }

    AudioFileDropAddOn::AudioFileDropAddOn(QObject *parent) : WindowInterfaceAddOn(parent) {
    }

    void AudioFileDropAddOn::initialize() {
        windowHandle()->addObject(this);
    }

    void AudioFileDropAddOn::extensionsInitialized() {
    }

    AudioFileDropAddOnAttachedType *AudioFileDropAddOn::qmlAttachedProperties(QObject *object) {
        return new AudioFileDropAddOnAttachedType(object);
    }

    bool AudioFileDropAddOn::isImporting() const {
        return m_importing;
    }

    bool AudioFileDropAddOn::acceptsUrls(const QList<QUrl> &urls) const {
        if (m_importing || urls.isEmpty()) {
            return false;
        }
        QSet<QString> extensions;
        for (const auto &extension : Audio::GlobalAudioContext::formatManager()->extensionHints()) {
            extensions.insert(extension.toLower());
        }
        for (const auto &url : urls) {
            if (!url.isValid() || !url.isLocalFile() || !extensions.contains(QFileInfo(url.toLocalFile()).suffix().toLower())) {
                return false;
            }
        }
        return true;
    }

    bool AudioFileDropAddOn::importFiles(const QList<QUrl> &urls, dspx::Track *firstTrack, int position) {
        if (!acceptsUrls(urls)) {
            return false;
        }
        const auto window = windowHandle()->cast<Core::ProjectWindowInterface>();
        QPointer<Core::DspxDocument> document = window->projectDocumentContext()->document();
        if (!document || document->transactionController()->isExclusiveToTransaction() || (firstTrack && firstTrack->trackList() != document->model()->tracks())) {
            return false;
        }
        m_importing = true;
        Q_EMIT importingChanged();
        QMetaObject::invokeMethod(this, [this, urls, document, firstTrack = QPointer<dspx::Track>(firstTrack), hadFirstTrack = firstTrack != nullptr, position] {
            importFilesNow(urls, document, firstTrack, hadFirstTrack, qMax(0, position));
        }, Qt::QueuedConnection);
        return true;
    }

    void AudioFileDropAddOn::importFilesNow(const QList<QUrl> &urls, QPointer<Core::DspxDocument> document, QPointer<dspx::Track> firstTrack, bool hadFirstTrack, int position) {
        QPointer<AudioFileDropAddOn> guard(this);
        const auto resetImporting = qScopeGuard([guard] {
            if (guard) {
                guard->m_importing = false;
                Q_EMIT guard->importingChanged();
            }
        });
        const QPointer<Core::ProjectWindowInterface> window = windowHandle()->cast<Core::ProjectWindowInterface>();
        if (!document || window->projectDocumentContext()->document() != document || (hadFirstTrack && !firstTrack)) {
            return;
        }
        const auto model = document->model();
        const auto trackList = model->tracks();
        QList<QPointer<dspx::Track>> targetTracks;
        if (firstTrack) {
            const auto tracks = trackList->items();
            const auto index = tracks.indexOf(firstTrack.data());
            if (index < 0) {
                return;
            }
            for (auto i = index; i < tracks.size(); ++i) {
                targetTracks.append(tracks.at(i));
            }
        }

        std::vector<std::unique_ptr<Audio::AudioClipImporter>> importers;
        QStringList failedFiles;
        qCDebug(lcAudioFileDropAddOn) << "Preparing dropped audio files" << urls.size();
        for (const auto &url : urls) {
            auto importer = std::make_unique<Audio::AudioClipImporter>(window);
            const auto opened = importer->openFile(url.toLocalFile());
            if (!guard || !window || !document || window->projectDocumentContext()->document() != document) {
                return;
            }
            if (opened) {
                importers.push_back(std::move(importer));
            } else {
                failedFiles.append(QDir::toNativeSeparators(url.toLocalFile()));
            }
        }

        if (!importers.empty()) {
            for (const auto &track : targetTracks) {
                if (!track || track->trackList() != trackList) {
                    return;
                }
            }
            const auto transaction = document->transactionController()->beginTransaction();
            if (transaction == Core::TransactionController::TransactionId::Invalid) {
                window->sendNotification(SVS::SVSCraft::Warning, tr("Audio import failed"), tr("Audio files cannot be imported while another edit is active."));
                return;
            }
            QList<dspx::AudioClip *> clips;
            bool succeeded = true;
            qCDebug(lcAudioFileDropAddOn) << "Inserting dropped audio files" << importers.size() << "at" << position;
            for (std::size_t i = 0; i < importers.size(); ++i) {
                auto &importer = importers[i];
                const auto name = QFileInfo(importer->fileName()).completeBaseName();
                dspx::Track *track;
                if (i < static_cast<std::size_t>(targetTracks.size())) {
                    track = targetTracks.at(static_cast<qsizetype>(i));
                } else {
                    track = model->createTrack();
                    track->fromOpenDSPX(opendspx::Track{});
                    track->setName(name);
                    const auto colors = Core::CoreInterface::trackColorSchema()->colors();
                    if (!colors.isEmpty()) {
                        const auto index = trackList->size() % colors.size();
                        track->setColorId(index % 2 ? (colors.size() + index) / 2 : index / 2);
                    }
                    if (!trackList->insertItem(trackList->size(), track)) {
                        model->destroyItem(track);
                        succeeded = false;
                        break;
                    }
                }
                auto clip = importer->insertClip(track, position, name);
                if (!clip) {
                    succeeded = false;
                    break;
                }
                clips.append(clip);
            }
            if (!succeeded) {
                document->transactionController()->abortTransaction(transaction);
                qCWarning(lcAudioFileDropAddOn) << "Failed to insert dropped audio clips";
                window->sendNotification(SVS::SVSCraft::Critical, tr("Audio import failed"), tr("The audio clips could not be inserted."));
                return;
            }
            document->transactionController()->commitTransaction(transaction, tr("Importing audio files"));
            qCDebug(lcAudioFileDropAddOn) << "Committed dropped audio clips" << clips.size();
            bool first = true;
            for (auto clip : std::as_const(clips)) {
                auto command = dspx::SelectionModel::Select | dspx::SelectionModel::SetCurrentItem;
                if (first) {
                    command |= dspx::SelectionModel::ClearPreviousSelection;
                    first = false;
                }
                document->selectionModel()->select(clip, command);
            }
        }
        if (!failedFiles.isEmpty()) {
            window->sendNotification(SVS::SVSCraft::Warning, tr("Audio files skipped"), tr("The following audio files could not be imported:\n%1").arg(failedFiles.join('\n')));
        }
    }

}

#include "moc_AudioFileDropAddOn.cpp"
