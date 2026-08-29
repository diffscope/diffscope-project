// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "EffectsAddOn.h"

#include <algorithm>
#include <utility>

#include <QAbstractItemModel>
#include <QCollator>
#include <QLoggingCategory>
#include <QQmlComponent>
#include <QQuickItem>
#include <QQuickWindow>
#include <QSet>
#include <QVariant>

#include <CoreApi/runtimeinterface.h>

#include <QAKQuick/quickactioncontext.h>

#include <dspxmodelORM/AnchorNodeSequence.h>
#include <dspxmodelORM/AudioDSP.h>
#include <dspxmodelORM/AudioDSPList.h>
#include <dspxmodelORM/Clip.h>
#include <dspxmodelORM/ClipSequence.h>
#include <dspxmodelORM/DynamicMixingAnchorSequence.h>
#include <dspxmodelORM/Model.h>
#include <dspxmodelORM/NoteSequence.h>
#include <dspxmodelORM/Parameter.h>
#include <dspxmodelORM/ParameterMap.h>
#include <dspxmodelORM/SingingClip.h>
#include <dspxmodelORM/Sources.h>
#include <dspxmodelORM/Track.h>
#include <dspxmodelORM/TrackList.h>
#include <dspxmodelSelectionModel/AnchorNodeSelectionModel.h>
#include <dspxmodelSelectionModel/ClipSelectionModel.h>
#include <dspxmodelSelectionModel/DynamicMixingAnchorSelectionModel.h>
#include <dspxmodelSelectionModel/NoteSelectionModel.h>
#include <dspxmodelSelectionModel/SelectionModel.h>
#include <dspxmodelSelectionModel/TrackSelectionModel.h>

#include <transactional/TransactionController.h>

#include <coreplugin/DspxDocument.h>
#include <coreplugin/ProjectDocumentContext.h>
#include <coreplugin/ProjectWindowInterface.h>

#include <audio/EffectsUnitClass.h>
#include <audio/EffectsUnitCollection.h>
#include <audio/ProjectAudioContext.h>
#include <audio/TrackAudioContext.h>
#include <audio/internal/EffectsContext.h>
#include <audio/internal/EffectsPresets.h>

namespace Audio::Internal {

    Q_STATIC_LOGGING_CATEGORY(lcEffectsAddOn, "diffscope.audio.effectsaddon")

    namespace {

        dspx::Track *trackFromSingingClip(dspx::SingingClip *singingClip) {
            if (!singingClip) {
                return nullptr;
            }
            auto clipSequence = singingClip->clipSequence();
            return clipSequence ? clipSequence->track() : nullptr;
        }

        dspx::Track *trackFromNoteSequence(dspx::NoteSequence *noteSequence) {
            return noteSequence ? trackFromSingingClip(noteSequence->singingClip()) : nullptr;
        }

        dspx::Track *trackFromAnchorNodeSequence(dspx::AnchorNodeSequence *anchorNodeSequence) {
            if (!anchorNodeSequence) {
                return nullptr;
            }
            auto parameter = anchorNodeSequence->parameter();
            auto parameterMap = parameter ? parameter->parameterMap() : nullptr;
            return parameterMap ? trackFromSingingClip(parameterMap->singingClip()) : nullptr;
        }

        dspx::Track *trackFromDynamicMixingAnchorSequence(
            dspx::DynamicMixingAnchorSequence *sequence) {
            if (!sequence) {
                return nullptr;
            }
            auto sources = sequence->sources();
            return sources ? trackFromSingingClip(sources->singingClip()) : nullptr;
        }

    }

    EffectsAddOn::EffectsAddOn(QObject *parent)
        : WindowInterfaceAddOn(parent) {
    }

    EffectsAddOn::~EffectsAddOn() {
        clearAssociationConnections();
    }

    void EffectsAddOn::initialize() {
        auto window = windowHandle()->cast<Core::ProjectWindowInterface>();
        window->addObject(this);
        auto document = window->projectDocumentContext()->document();
        createMasterContext();
        auto trackList = document->model()->tracks();
        for (auto track : trackList->items()) {
            createTrackContext(track);
        }
        connect(trackList, &dspx::TrackList::itemInserted, this,
                [this](int, dspx::Track *track) {
                    createTrackContext(track);
                    refreshSelection();
                });
        connect(trackList, &dspx::TrackList::itemRemoved, this,
                [this](int, dspx::Track *) {
                    refreshSelection();
                });

        m_selectionModel = document->selectionModel();
        connect(m_selectionModel, &dspx::SelectionModel::selectionTypeChanged,
                this, &EffectsAddOn::refreshSelection);
        connect(m_selectionModel, &dspx::SelectionModel::selectedCountChanged,
                this, &EffectsAddOn::refreshSelection);
        connect(m_selectionModel, &dspx::SelectionModel::itemSelected,
                this, [this](QObject *, bool) {
                    refreshSelection();
                });
        connect(m_selectionModel->clipSelectionModel(),
                &dspx::ClipSelectionModel::clipSequencesWithSelectedItemsChanged, this,
                &EffectsAddOn::refreshSelection);
        connect(m_selectionModel->noteSelectionModel(),
                &dspx::NoteSelectionModel::noteSequenceWithSelectedItemsChanged, this,
                &EffectsAddOn::refreshSelection);
        connect(m_selectionModel->anchorNodeSelectionModel(),
                &dspx::AnchorNodeSelectionModel::anchorNodeSequenceWithSelectedItemsChanged, this,
                &EffectsAddOn::refreshSelection);
        connect(m_selectionModel->dynamicMixingAnchorSelectionModel(),
                &dspx::DynamicMixingAnchorSelectionModel::dynamicMixingAnchorSequenceWithSelectedItemsChanged,
                this, &EffectsAddOn::refreshSelection);

        auto collection = EffectsUnitCollection::instance();
        connect(collection, &EffectsUnitCollection::effectsUnitIdsChanged,
                this, &EffectsAddOn::refreshAvailableEffects);
        refreshAvailableEffects();

        connect(EffectsPresets::instance(), &EffectsPresets::presetsChanged,
                this, &EffectsAddOn::presetsChanged);

        refreshSelection();

        QQmlComponent component(Core::RuntimeInterface::qmlEngine(),
                                QStringLiteral("DiffScope.Audio"),
                                QStringLiteral("EffectsPanel"), this);
        if (component.isError()) {
            qFatal() << component.errorString();
        }
        auto object = component.createWithInitialProperties({
            {QStringLiteral("addOn"), QVariant::fromValue(this)},
        }, component.creationContext());
        if (!object) {
            qFatal() << component.errorString();
        }
        object->setParent(this);
        window->actionContext()->addAction(
            QStringLiteral("org.diffscope.audio.panel.effects"),
            object->property("panelComponent").value<QQmlComponent *>());
    }

    void EffectsAddOn::extensionsInitialized() {
    }

    bool EffectsAddOn::delayedInitialize() {
        return WindowInterfaceAddOn::delayedInitialize();
    }

    EffectsAddOn *EffectsAddOn::of(Core::ProjectWindowInterface *windowHandle) {
        return windowHandle->getFirstObject<EffectsAddOn>();
    }

    void EffectsAddOn::refreshAllEffects() {
        if (m_masterContext) {
            m_masterContext->refreshEffects();
        }
        for (const auto &context : std::as_const(m_trackContexts)) {
            if (context) {
                context->refreshEffects();
            }
        }
    }

    void EffectsAddOn::beginEffectsBypass() {
        ++m_effectsBypassDepth;
        if (m_effectsBypassDepth != 1) {
            return;
        }
        if (m_masterContext) {
            m_masterContext->setEffectsBypassed(true);
        }
        for (const auto &context : std::as_const(m_trackContexts)) {
            if (context) {
                context->setEffectsBypassed(true);
            }
        }
    }

    void EffectsAddOn::endEffectsBypass() {
        if (m_effectsBypassDepth == 0) {
            return;
        }
        --m_effectsBypassDepth;
        if (m_effectsBypassDepth != 0) {
            return;
        }
        if (m_masterContext) {
            m_masterContext->setEffectsBypassed(false);
        }
        for (const auto &context : std::as_const(m_trackContexts)) {
            if (context) {
                context->setEffectsBypassed(false);
            }
        }
    }

    QAbstractItemModel *EffectsAddOn::effectsModel() const {
        return activeContext();
    }

    QString EffectsAddOn::selectionMessage() const {
        return m_activeTab == TrackTab ? m_trackSelectionMessage : m_masterSelectionMessage;
    }

    bool EffectsAddOn::hasEffectsContext() const {
        return activeContext() != nullptr;
    }

    bool EffectsAddOn::trackTabVisible() const {
        return m_trackTabVisible;
    }

    QString EffectsAddOn::trackTabText() const {
        return m_selectedTrack ? tr("Track: %1").arg(m_selectedTrack->name()) : tr("Track");
    }

    int EffectsAddOn::activeTab() const {
        return m_activeTab;
    }

    void EffectsAddOn::setActiveTab(int activeTab) {
        if ((activeTab != TrackTab && activeTab != MasterTab) ||
            (activeTab == TrackTab && !m_trackTabVisible) ||
            m_activeTab == activeTab) {
            return;
        }
        m_activeTab = activeTab;
        Q_EMIT selectionContextChanged();
    }

    bool EffectsAddOn::readingFilterConflict() const {
        auto context = activeContext();
        return context && context->readingFilterConflict();
    }

    QString EffectsAddOn::readingFilterConflictMessage() const {
        return m_activeTab == TrackTab
            ? tr("Another audio reading filter is already attached to this track. Effects cannot process audio on this track.")
            : tr("Another audio reading filter is already attached to the master track. Effects cannot process master audio.");
    }

    QVariantList EffectsAddOn::availableEffects() const {
        return m_availableEffects;
    }

    bool EffectsAddOn::addEffect(const QString &id) {
        auto context = activeContext();
        return context && context->addEffect(id);
    }

    bool EffectsAddOn::removeEffect(int row) {
        auto context = activeContext();
        return context && context->removeEffect(row);
    }

    bool EffectsAddOn::setEffectEnabled(int row, bool enabled) {
        auto context = activeContext();
        return context && context->setEffectEnabled(row, enabled);
    }

    bool EffectsAddOn::moveEffect(int row, int offset) {
        auto context = activeContext();
        return context && context->moveEffect(row, offset);
    }

    bool EffectsAddOn::resetEffect(int row) {
        auto context = activeContext();
        return context && context->resetEffect(row);
    }

    void EffectsAddOn::setExpanded(int row, bool expanded) {
        if (auto context = activeContext()) {
            context->setExpanded(row, expanded);
        }
    }

    QStringList EffectsAddOn::presetNames() const {
        return EffectsPresets::instance()->presetNames();
    }

    bool EffectsAddOn::savePreset(const QString &name) {
        auto context = activeContext();
        if (!context) {
            return false;
        }
        return EffectsPresets::instance()->savePreset(name, context->audioDSPList()->toOpenDSPX());
    }

    bool EffectsAddOn::applyPreset(const QString &name) {
        auto context = activeContext();
        if (!context) {
            return false;
        }
        const auto array = EffectsPresets::instance()->presetAudioDSPs(name);
        auto document = windowHandle()->cast<Core::ProjectWindowInterface>()->projectDocumentContext()->document();
        bool applied = false;
        const bool transactionStarted = document->transactionController()->beginScopedTransaction(
            tr("Applying effect preset"), [context, array, &applied] {
                const auto oldItems = context->audioDSPList()->items();
                context->audioDSPList()->fromOpenDSPX(array);
                for (auto *oldItem : oldItems) {
                    oldItem->model()->destroyItem(oldItem);
                }
                applied = true;
                return true;
            });
        return transactionStarted && applied;
    }

    bool EffectsAddOn::deletePreset(const QString &name) {
        return EffectsPresets::instance()->removePreset(name);
    }

    bool EffectsAddOn::hasPreset(const QString &name) {
        return EffectsPresets::instance()->hasPreset(name);
    }

    int EffectsAddOn::presetIndex(const QString &name) {
        return EffectsPresets::instance()->presetNames().indexOf(name);
    }

    void EffectsAddOn::openPresetDialog() {
        auto window = windowHandle()->cast<Core::ProjectWindowInterface>();
        auto quickWindow = qobject_cast<QQuickWindow *>(window->window());
        if (!quickWindow) {
            return;
        }
        QQmlComponent component(Core::RuntimeInterface::qmlEngine(),
                                QStringLiteral("DiffScope.Audio"),
                                QStringLiteral("EffectsPresetDialog"), this);
        if (component.isError()) {
            qFatal() << component.errorString();
        }
        QVariantMap properties{{QStringLiteral("addOn"), QVariant::fromValue(this)}};
        properties.insert(QStringLiteral("parent"), QVariant::fromValue(quickWindow->contentItem()));
        auto object = component.createWithInitialProperties(properties);
        if (!object) {
            qFatal() << component.errorString();
        }
        const auto width = object->property("width").toDouble();
        const auto height = object->property("height").toDouble();
        object->setProperty("x", quickWindow->width() / 2.0 - width / 2);
        if (auto popupTopMarginHint = quickWindow->property("popupTopMarginHint"); popupTopMarginHint.isValid()) {
            object->setProperty("y", popupTopMarginHint);
        } else {
            object->setProperty("y", quickWindow->height() / 2.0 - height / 2);
        }
        QMetaObject::invokeMethod(object, "open");
    }

    EffectsContext *EffectsAddOn::activeContext() const {
        return m_activeTab == TrackTab ? m_trackContext.data() : m_masterContext.data();
    }

    void EffectsAddOn::createMasterContext() {
        auto window = windowHandle()->cast<Core::ProjectWindowInterface>();
        auto audioContext = Audio::ProjectAudioContext::of(window);
        if (!audioContext) {
            m_masterSelectionMessage = tr("Effects are unavailable for the master track.");
            qCWarning(lcEffectsAddOn) << "Project audio context is unavailable";
            return;
        }
        auto document = window->projectDocumentContext()->document();
        m_masterContext = new EffectsContext(window,
                                             document->model()->audioDSPs(),
                                             audioContext->masterTrackMixer(),
                                             audioContext);
        m_masterContext->setEffectsBypassed(m_effectsBypassDepth != 0);
        connect(m_masterContext, &QObject::destroyed, this, [this] {
            m_masterContext = nullptr;
            m_masterSelectionMessage = tr("Effects are unavailable for the master track.");
            Q_EMIT selectionContextChanged();
        });
    }

    void EffectsAddOn::createTrackContext(dspx::Track *track) {
        if (!track || m_trackContexts.contains(track)) {
            return;
        }
        auto audioContext = Audio::TrackAudioContext::of(track);
        if (!audioContext) {
            qCWarning(lcEffectsAddOn) << "Track audio context is unavailable" << track;
            return;
        }
        auto context = new EffectsContext(audioContext->windowHandle(),
                                          track->audioDSPs(),
                                          audioContext->trackMixer(),
                                          audioContext);
        context->setEffectsBypassed(m_effectsBypassDepth != 0);
        m_trackContexts.insert(track, context);
        connect(context, &QObject::destroyed, this, [this, track] {
            m_trackContexts.remove(track);
            if (m_selectedTrack == track) {
                m_trackContext = nullptr;
                m_trackSelectionMessage = tr("Effects are unavailable for this track.");
                Q_EMIT selectionContextChanged();
            }
        });
    }

    void EffectsAddOn::refreshSelection() {
        clearAssociationConnections();
        if (!m_selectionModel) {
            setTrackSelection(false, nullptr, nullptr, {});
            return;
        }

        QSet<dspx::Track *> tracks;
        bool mappingFailed = false;
        const auto addTrack = [&tracks, &mappingFailed](dspx::Track *track) {
            if (track) {
                tracks.insert(track);
            } else {
                mappingFailed = true;
            }
        };
        const auto watch = [this]<typename Sender, typename Signal>(Sender *sender, Signal signal) {
            if (sender) {
                m_associationConnections.append(connect(sender, signal, this, [this] {
                    refreshSelection();
                }));
            }
        };

        switch (m_selectionModel->selectionType()) {
            case dspx::SelectionModel::ST_Track:
                for (auto track : m_selectionModel->trackSelectionModel()->selectedItems()) {
                    addTrack(track);
                }
                break;
            case dspx::SelectionModel::ST_Clip:
                for (auto clipSequence :
                     m_selectionModel->clipSelectionModel()->clipSequencesWithSelectedItems()) {
                    addTrack(clipSequence ? clipSequence->track() : nullptr);
                }
                break;
            case dspx::SelectionModel::ST_Note: {
                auto noteSequence =
                    m_selectionModel->noteSelectionModel()->noteSequenceWithSelectedItems();
                if (noteSequence) {
                    addTrack(trackFromNoteSequence(noteSequence));
                    watch(noteSequence->singingClip(), &dspx::Clip::clipSequenceChanged);
                }
                break;
            }
            case dspx::SelectionModel::ST_AnchorNode: {
                auto anchorNodeSequence = m_selectionModel->anchorNodeSelectionModel()
                                              ->anchorNodeSequenceWithSelectedItems();
                if (anchorNodeSequence) {
                    addTrack(trackFromAnchorNodeSequence(anchorNodeSequence));
                    auto parameter = anchorNodeSequence->parameter();
                    watch(parameter, &dspx::Parameter::parameterMapChanged);
                    auto parameterMap = parameter ? parameter->parameterMap() : nullptr;
                    watch(parameterMap ? parameterMap->singingClip() : nullptr,
                          &dspx::Clip::clipSequenceChanged);
                }
                break;
            }
            case dspx::SelectionModel::ST_DynamicMixingAnchor: {
                auto sequence = m_selectionModel->dynamicMixingAnchorSelectionModel()
                                    ->dynamicMixingAnchorSequenceWithSelectedItems();
                if (sequence) {
                    addTrack(trackFromDynamicMixingAnchorSequence(sequence));
                    auto sources = sequence->sources();
                    watch(sources, &dspx::Sources::singingClipChanged);
                    watch(sources ? sources->singingClip() : nullptr,
                          &dspx::Clip::clipSequenceChanged);
                }
                break;
            }
            default:
                setTrackSelection(false, nullptr, nullptr, {});
                return;
        }

        if (tracks.size() > 1) {
            setTrackSelection(true, nullptr, nullptr, tr("Effects cannot be edited for multiple tracks."));
            return;
        }
        if (mappingFailed || tracks.isEmpty()) {
            setTrackSelection(false, nullptr, nullptr, {});
            return;
        }
        auto track = *tracks.constBegin();
        auto context = m_trackContexts.value(track);
        if (!context) {
            setTrackSelection(true, track, nullptr, tr("Effects are unavailable for this track."));
            return;
        }
        setTrackSelection(true, track, context, {});
    }

    void EffectsAddOn::refreshAvailableEffects() {
        struct Item {
            QString id;
            QString name;
        };
        QList<Item> items;
        auto collection = EffectsUnitCollection::instance();
        for (const auto &id : collection->effectsUnitIds()) {
            if (auto effectsUnitClass = collection->effectsUnitClass(id)) {
                items.append({id, effectsUnitClass->name()});
            }
        }
        QCollator collator;
        collator.setNumericMode(true);
        std::ranges::sort(items, [&collator](const Item &left, const Item &right) {
            const int nameComparison = collator.compare(left.name, right.name);
            return nameComparison == 0
                ? collator.compare(left.id, right.id) < 0
                : nameComparison < 0;
        });
        QVariantList result;
        result.reserve(items.size());
        for (const auto &item : std::as_const(items)) {
            result.append(QVariantMap{
                {QStringLiteral("id"), item.id},
                {QStringLiteral("name"), item.name},
            });
        }
        if (m_availableEffects == result) {
            return;
        }
        m_availableEffects = std::move(result);
        Q_EMIT availableEffectsChanged();
    }

    void EffectsAddOn::setTrackSelection(bool tabVisible,
                                              dspx::Track *track,
                                              EffectsContext *context,
                                              const QString &message) {
        m_trackTabVisible = tabVisible;
        m_selectedTrack = track;
        m_trackContext = context;
        m_trackSelectionMessage = message;
        m_activeTab = tabVisible ? TrackTab : MasterTab;
        if (track) {
            m_associationConnections.append(connect(track, &dspx::Track::nameChanged, this, [this, track] {
                if (m_selectedTrack == track) {
                    Q_EMIT selectionContextChanged();
                }
            }));
        }
        Q_EMIT selectionContextChanged();
    }

    void EffectsAddOn::clearAssociationConnections() {
        for (const auto &connection : std::as_const(m_associationConnections)) {
            disconnect(connection);
        }
        m_associationConnections.clear();
    }

}

#include "moc_EffectsAddOn.cpp"
