// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "EffectsPanelAddOn.h"

#include <algorithm>
#include <utility>

#include <QAbstractItemModel>
#include <QCollator>
#include <QLoggingCategory>
#include <QQmlComponent>
#include <QSet>
#include <QVariant>

#include <CoreApi/runtimeinterface.h>

#include <QAKQuick/quickactioncontext.h>

#include <dspxmodelORM/AnchorNode.h>
#include <dspxmodelORM/AnchorNodeSequence.h>
#include <dspxmodelORM/Clip.h>
#include <dspxmodelORM/ClipSequence.h>
#include <dspxmodelORM/DynamicMixingAnchor.h>
#include <dspxmodelORM/DynamicMixingAnchorSequence.h>
#include <dspxmodelORM/Model.h>
#include <dspxmodelORM/Note.h>
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

#include <audio/TrackAudioContext.h>

#include <coreplugin/DspxDocument.h>
#include <coreplugin/ProjectDocumentContext.h>
#include <coreplugin/ProjectWindowInterface.h>

#include <effectsunitmanager/EffectsUnitClass.h>
#include <effectsunitmanager/EffectsUnitCollection.h>
#include <effectsunitmanager/internal/TrackEffectsContext.h>

namespace EffectsUnitManager::Internal {

    Q_STATIC_LOGGING_CATEGORY(lcEffectsPanelAddOn, "diffscope.effectsunitmanager.effectspaneladdon")

    namespace {

        dspx::Track *trackFromClip(dspx::Clip *clip) {
            return clip && clip->clipSequence() ? clip->clipSequence()->track() : nullptr;
        }

        dspx::Track *trackFromSingingClip(dspx::SingingClip *singingClip) {
            return singingClip ? trackFromClip(singingClip) : nullptr;
        }

        dspx::Track *trackFromNote(dspx::Note *note) {
            return note && note->noteSequence()
                ? trackFromSingingClip(note->noteSequence()->singingClip())
                : nullptr;
        }

        dspx::Track *trackFromAnchorNode(dspx::AnchorNode *anchor) {
            if (!anchor || !anchor->anchorNodeSequence()) {
                return nullptr;
            }
            auto parameter = anchor->anchorNodeSequence()->parameter();
            auto parameterMap = parameter ? parameter->parameterMap() : nullptr;
            return parameterMap ? trackFromSingingClip(parameterMap->singingClip()) : nullptr;
        }

        dspx::Track *trackFromDynamicMixingAnchor(dspx::DynamicMixingAnchor *anchor) {
            if (!anchor || !anchor->dynamicMixingAnchorSequence()) {
                return nullptr;
            }
            auto sources = anchor->dynamicMixingAnchorSequence()->sources();
            return sources ? trackFromSingingClip(sources->singingClip()) : nullptr;
        }

    }

    EffectsPanelAddOn::EffectsPanelAddOn(QObject *parent)
        : WindowInterfaceAddOn(parent) {
    }

    EffectsPanelAddOn::~EffectsPanelAddOn() {
        clearAssociationConnections();
    }

    void EffectsPanelAddOn::initialize() {
        auto window = windowHandle()->cast<Core::ProjectWindowInterface>();
        window->addObject(this);
        auto document = window->projectDocumentContext()->document();
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
                this, &EffectsPanelAddOn::refreshSelection);
        connect(m_selectionModel, &dspx::SelectionModel::selectedCountChanged,
                this, &EffectsPanelAddOn::refreshSelection);
        connect(m_selectionModel, &dspx::SelectionModel::itemSelected,
                this, [this](QObject *, bool) {
                    refreshSelection();
                });

        auto collection = EffectsUnitCollection::instance();
        connect(collection, &EffectsUnitCollection::effectsUnitIdsChanged,
                this, &EffectsPanelAddOn::refreshAvailableEffects);
        refreshAvailableEffects();
        refreshSelection();

        QQmlComponent component(Core::RuntimeInterface::qmlEngine(),
                                QStringLiteral("DiffScope.EffectsUnitManager"),
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
            QStringLiteral("org.diffscope.effectsunitmanager.panel.effects"),
            object->property("panelComponent").value<QQmlComponent *>());
    }

    void EffectsPanelAddOn::extensionsInitialized() {
    }

    bool EffectsPanelAddOn::delayedInitialize() {
        return WindowInterfaceAddOn::delayedInitialize();
    }

    QAbstractItemModel *EffectsPanelAddOn::effectsModel() const {
        return m_currentContext;
    }

    QString EffectsPanelAddOn::selectionMessage() const {
        return m_selectionMessage;
    }

    bool EffectsPanelAddOn::hasTrack() const {
        return !m_currentContext.isNull();
    }

    bool EffectsPanelAddOn::readingFilterConflict() const {
        return m_currentContext && m_currentContext->readingFilterConflict();
    }

    QVariantList EffectsPanelAddOn::availableEffects() const {
        return m_availableEffects;
    }

    bool EffectsPanelAddOn::addEffect(const QString &id) {
        return m_currentContext && m_currentContext->addEffect(id);
    }

    bool EffectsPanelAddOn::removeEffect(int row) {
        return m_currentContext && m_currentContext->removeEffect(row);
    }

    bool EffectsPanelAddOn::setEffectEnabled(int row, bool enabled) {
        return m_currentContext && m_currentContext->setEffectEnabled(row, enabled);
    }

    bool EffectsPanelAddOn::moveEffect(int row, int offset) {
        return m_currentContext && m_currentContext->moveEffect(row, offset);
    }

    void EffectsPanelAddOn::setExpanded(int row, bool expanded) {
        if (m_currentContext) {
            m_currentContext->setExpanded(row, expanded);
        }
    }

    void EffectsPanelAddOn::createTrackContext(dspx::Track *track) {
        if (!track || TrackEffectsContext::of(track)) {
            return;
        }
        auto audioContext = Audio::TrackAudioContext::of(track);
        if (!audioContext) {
            qCWarning(lcEffectsPanelAddOn) << "Track audio context is unavailable" << track;
            return;
        }
        new TrackEffectsContext(audioContext);
    }

    void EffectsPanelAddOn::refreshSelection() {
        clearAssociationConnections();
        if (!m_selectionModel || m_selectionModel->selectedCount() == 0) {
            setCurrentContext(nullptr, tr("Select a track or an item on a track to edit effects."));
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
                for (auto clip : m_selectionModel->clipSelectionModel()->selectedItems()) {
                    addTrack(trackFromClip(clip));
                    watch(clip, &dspx::Clip::clipSequenceChanged);
                }
                break;
            case dspx::SelectionModel::ST_Note:
                for (auto note : m_selectionModel->noteSelectionModel()->selectedItems()) {
                    addTrack(trackFromNote(note));
                    watch(note, &dspx::Note::noteSequenceChanged);
                    auto sequence = note->noteSequence();
                    auto singingClip = sequence ? sequence->singingClip() : nullptr;
                    watch(singingClip, &dspx::Clip::clipSequenceChanged);
                }
                break;
            case dspx::SelectionModel::ST_AnchorNode:
                for (auto anchor : m_selectionModel->anchorNodeSelectionModel()->selectedItems()) {
                    addTrack(trackFromAnchorNode(anchor));
                    watch(anchor, &dspx::AnchorNode::anchorNodeSequenceChanged);
                    auto sequence = anchor->anchorNodeSequence();
                    auto parameter = sequence ? sequence->parameter() : nullptr;
                    watch(parameter, &dspx::Parameter::parameterMapChanged);
                    auto parameterMap = parameter ? parameter->parameterMap() : nullptr;
                    auto singingClip = parameterMap ? parameterMap->singingClip() : nullptr;
                    watch(singingClip, &dspx::Clip::clipSequenceChanged);
                }
                break;
            case dspx::SelectionModel::ST_DynamicMixingAnchor:
                for (auto anchor : m_selectionModel->dynamicMixingAnchorSelectionModel()->selectedItems()) {
                    addTrack(trackFromDynamicMixingAnchor(anchor));
                    watch(anchor, &dspx::DynamicMixingAnchor::dynamicMixingAnchorSequenceChanged);
                    auto sequence = anchor->dynamicMixingAnchorSequence();
                    auto sources = sequence ? sequence->sources() : nullptr;
                    watch(sources, &dspx::Sources::singingClipChanged);
                    auto singingClip = sources ? sources->singingClip() : nullptr;
                    watch(singingClip, &dspx::Clip::clipSequenceChanged);
                }
                break;
            default:
                setCurrentContext(nullptr, tr("The current selection cannot be associated with a track."));
                return;
        }

        if (tracks.size() > 1) {
            setCurrentContext(nullptr, tr("Effects cannot be edited for multiple tracks."));
            return;
        }
        if (mappingFailed || tracks.isEmpty()) {
            setCurrentContext(nullptr, tr("The current selection cannot be associated with a track."));
            return;
        }
        auto context = TrackEffectsContext::of(*tracks.constBegin());
        if (!context) {
            setCurrentContext(nullptr, tr("Effects are unavailable for this track."));
            return;
        }
        setCurrentContext(context, {});
    }

    void EffectsPanelAddOn::refreshAvailableEffects() {
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

    void EffectsPanelAddOn::setCurrentContext(TrackEffectsContext *context, const QString &message) {
        if (m_currentContext == context && m_selectionMessage == message) {
            return;
        }
        if (m_contextDestroyedConnection) {
            disconnect(m_contextDestroyedConnection);
            m_contextDestroyedConnection = {};
        }
        m_currentContext = context;
        m_selectionMessage = message;
        if (context) {
            m_contextDestroyedConnection = connect(context, &QObject::destroyed, this, [this] {
                m_currentContext = nullptr;
                m_selectionMessage = tr("Effects are unavailable for this track.");
                Q_EMIT selectionContextChanged();
            });
        }
        Q_EMIT selectionContextChanged();
    }

    void EffectsPanelAddOn::clearAssociationConnections() {
        for (const auto &connection : std::as_const(m_associationConnections)) {
            disconnect(connection);
        }
        m_associationConnections.clear();
    }

}

#include "moc_EffectsPanelAddOn.cpp"
