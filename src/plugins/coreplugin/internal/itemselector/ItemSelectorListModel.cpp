// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#include "ItemSelectorListModel.h"

#include <algorithm>
#include <initializer_list>
#include <utility>

#include <QDir>
#include <QHash>
#include <QLocale>
#include <QMap>
#include <QMetaType>
#include <QPointer>

#include <SVSCraftCore/DecibelLinearizer.h>
#include <SVSCraftCore/MusicMode.h>
#include <SVSCraftCore/MusicModeInfo.h>
#include <SVSCraftCore/MusicPitch.h>
#include <SVSCraftCore/MusicTime.h>
#include <SVSCraftCore/MusicTimeline.h>

#include <dspxmodelORM/AnchorNode.h>
#include <dspxmodelORM/AnchorNodeSequence.h>
#include <dspxmodelORM/AudioClip.h>
#include <dspxmodelORM/Clip.h>
#include <dspxmodelORM/ClipSequence.h>
#include <dspxmodelORM/DynamicMixingAnchor.h>
#include <dspxmodelORM/DynamicMixingAnchorSequence.h>
#include <dspxmodelORM/KeySignature.h>
#include <dspxmodelORM/KeySignatureSequence.h>
#include <dspxmodelORM/Label.h>
#include <dspxmodelORM/LabelSequence.h>
#include <dspxmodelORM/Model.h>
#include <dspxmodelORM/Note.h>
#include <dspxmodelORM/NoteSequence.h>
#include <dspxmodelORM/Parameter.h>
#include <dspxmodelORM/ParameterMap.h>
#include <dspxmodelORM/SingingClip.h>
#include <dspxmodelORM/Sources.h>
#include <dspxmodelORM/Tempo.h>
#include <dspxmodelORM/TempoSequence.h>
#include <dspxmodelORM/Track.h>
#include <dspxmodelORM/TrackList.h>
#include <dspxmodelSelectionModel/AnchorNodeSelectionModel.h>
#include <dspxmodelSelectionModel/ClipSelectionModel.h>
#include <dspxmodelSelectionModel/DynamicMixingAnchorSelectionModel.h>
#include <dspxmodelSelectionModel/NoteSelectionModel.h>
#include <dspxmodelSelectionModel/SelectionModel.h>

#include <coreplugin/ArchitectureInfo.h>
#include <coreplugin/ClipSingerIdProvider.h>
#include <coreplugin/CoreInterface.h>
#include <coreplugin/DspxDocument.h>
#include <coreplugin/ParameterInfoProvider.h>
#include <coreplugin/ProjectDocumentContext.h>
#include <coreplugin/ProjectTimeline.h>
#include <coreplugin/ProjectWindowInterface.h>
#include <coreplugin/SingerInfo.h>
#include <coreplugin/SingerInfoProvider.h>
#include <coreplugin/internal/BehaviorPreference.h>
#include <coreplugin/internal/ItemSelectorDialog.h>
#include <coreplugin/internal/KeySignatureAtSpecifiedPositionHelper.h>

namespace Core::Internal {

    bool ItemSelectorEntry::matches(const ItemSelectorEntry &other) const {
        return kind == other.kind && object == other.object && key == other.key;
    }

    namespace {

    QList<int> rolesIncludingDescription(std::initializer_list<int> additionalRoles = {}) {
        QList<int> roles(additionalRoles);
        roles.append(Qt::ToolTipRole);
        roles.append(Qt::AccessibleDescriptionRole);
        roles.append(ItemSelectorListModel::DescriptionRole);
        return roles;
    }

    QString yesNo(bool value) {
        return value ? ItemSelectorDialog::tr("Yes") : ItemSelectorDialog::tr("No");
    }

    QString formatOffset(int tick) {
        return SVS::MusicTimeOffset(tick).toString(1, 3);
    }

    QString formatPosition(SVS::MusicTimeline *timeline, int tick) {
        return timeline ? timeline->create(0, 0, tick).toString(1, 1, 3) : QString{};
    }

    QString interpolationText(dspx::AnchorNode::InterpolationMode mode) {
        switch (mode) {
            case dspx::AnchorNode::None:
                return ItemSelectorDialog::tr("None");
            case dspx::AnchorNode::Linear:
                return ItemSelectorDialog::tr("Linear");
            case dspx::AnchorNode::Hermite:
                return ItemSelectorDialog::tr("Hermite");
        }
        return {};
    }

    QString musicModeName(int mode) {
        static const auto modeNames = [] {
            QMap<int, QString> result;
            for (const auto &[musicMode, name] : SVS::MusicModeInfo::getBuiltInMusicModeInfoList()) {
                result.insert(musicMode.mask(), name);
            }
            return result;
        }();
        auto modeName = modeNames.value(mode);
        if (modeName.isEmpty()) {
            modeName = ItemSelectorDialog::tr("Custom Mode");
        }
        return modeName;
    }

    QString keySignatureText(const dspx::KeySignature *keySignature) {
        const auto modeName = musicModeName(keySignature->mode());
        if (keySignature->mode() == 0) {
            return modeName;
        }
        auto keyName = SVS::MusicPitch(static_cast<qint8>(keySignature->tonality()))
                           .toString(static_cast<SVS::MusicPitch::Accidental>(keySignature->accidentalType()));
        keyName.chop(1);
        return QStringLiteral("%1 %2").arg(keyName, modeName);
    }

    QString firstLeafPath(const QVariantList &tree, const QString &parentPath) {
        for (int index = 0; index < tree.size(); ++index) {
            const auto path = parentPath.isEmpty()
                                  ? QString::number(index)
                                  : QStringLiteral("%1/%2").arg(parentPath).arg(index);
            const auto node = tree.at(index);
            if (node.metaType().id() == QMetaType::QString) {
                return path;
            }
            const auto result = firstLeafPath(node.toList(), path);
            if (!result.isEmpty()) {
                return result;
            }
        }
        return {};
    }

    class SingerNameResolver : public QObject {
        Q_OBJECT

    public:
        explicit SingerNameResolver(dspx::SingingClip *clip, QObject *parent = nullptr,
                                    bool languageAware = false)
            : QObject(parent), m_clip(clip), m_clipProvider(new ClipSingerIdProvider(this)) {
            connect(m_clipProvider, &ClipSingerIdProvider::architectureIdChanged, this, &SingerNameResolver::rebuildProviders);
            connect(m_clipProvider, &ClipSingerIdProvider::singerTreeChanged, this, &SingerNameResolver::rebuildProviders);
            if (languageAware) {
                connect(BehaviorPreference::instance(), &BehaviorPreference::fallbackLyricLanguageCodeChanged,
                        this, &SingerNameResolver::changed);
            }
            if (m_clip) {
                connect(m_clip, &dspx::SingingClip::sourcesChanged, this, [this] {
                    m_clipProvider->setSources(m_clip ? m_clip->sources() : nullptr);
                });
            }
            m_clipProvider->setSources(m_clip ? m_clip->sources() : nullptr);
            rebuildProviders();
        }

        QString displayName() const {
            const auto tree = m_clipProvider->singerTree();
            if (tree.isEmpty()) {
                return tr("No singer");
            }

            QStringList names;
            names.reserve(tree.size());
            for (int index = 0; index < tree.size(); ++index) {
                names.append(formatNode(tree.at(index), QString::number(index)));
            }
            if (names.size() > 1) {
                return tr("Mixed singer (%1)").arg(names.join(tr(", ")));
            }
            return names.constFirst();
        }

        QString languageName(const QString &languageCode) const {
            const auto firstPath = firstLeafPath(m_clipProvider->singerTree(), QString{});
            auto *provider = m_providers.value(firstPath);
            if (!provider || !provider->exists() || provider->info().languages().isEmpty()) {
                return languageCode == BehaviorPreference::fallbackLyricLanguageCode()
                           ? languageCode
                           : tr("Custom (%1)").arg(languageCode);
            }

            const auto languages = provider->info().languages();
            const auto it = languages.constFind(languageCode);
            if (it == languages.cend()) {
                return tr("Custom (%1)").arg(languageCode);
            }
            return it->name.isEmpty() ? languageCode : it->name;
        }

        QStringList dynamicSingerNames() const {
            QStringList result;
            const auto tree = m_clipProvider->singerTree();
            result.reserve(tree.size());
            for (int index = 0; index < tree.size(); ++index) {
                const auto node = tree.at(index);
                if (node.metaType().id() == QMetaType::QString) {
                    result.append(resolvedName(QString::number(index), node.toString()));
                } else {
                    result.append(tr("Mixed singer"));
                }
            }
            return result;
        }

        QString architectureId() const {
            return m_clipProvider->architectureId();
        }

    Q_SIGNALS:
        void changed();

    private:
        void rebuildProviders() {
            qDeleteAll(m_providers);
            m_providers.clear();
            addProviders(m_clipProvider->singerTree(), QString{});
            Q_EMIT changed();
        }

        void addProviders(const QVariantList &tree, const QString &parentPath) {
            for (int index = 0; index < tree.size(); ++index) {
                const auto path = parentPath.isEmpty()
                                      ? QString::number(index)
                                      : QStringLiteral("%1/%2").arg(parentPath).arg(index);
                const auto node = tree.at(index);
                if (node.metaType().id() == QMetaType::QString) {
                    auto *provider = new SingerInfoProvider(this);
                    provider->setRegistry(CoreInterface::singerRegistry());
                    provider->setArchitectureId(m_clipProvider->architectureId());
                    provider->setSingerId(node.toString());
                    connect(provider, &SingerInfoProvider::infoChanged, this, &SingerNameResolver::changed);
                    connect(provider, &SingerInfoProvider::existsChanged, this, &SingerNameResolver::changed);
                    connect(provider, &SingerInfoProvider::languageOptionsChanged, this, &SingerNameResolver::changed);
                    m_providers.insert(path, provider);
                } else {
                    addProviders(node.toList(), path);
                }
            }
        }

        QString formatNode(const QVariant &node, const QString &path) const {
            if (node.metaType().id() == QMetaType::QString) {
                return resolvedName(path, node.toString());
            }

            const auto children = node.toList();
            QStringList names;
            names.reserve(children.size());
            for (int index = 0; index < children.size(); ++index) {
                names.append(formatNode(children.at(index), QStringLiteral("%1/%2").arg(path).arg(index)));
            }
            return tr("Mixed singer (%1)").arg(names.join(tr(", ")));
        }

        QString resolvedName(const QString &path, const QString &singerId) const {
            auto *provider = m_providers.value(path);
            if (!provider) {
                return singerId;
            }
            const auto name = provider->info().name();
            return name.isEmpty() ? singerId : name;
        }

        QPointer<dspx::SingingClip> m_clip;
        ClipSingerIdProvider *m_clipProvider;
        QHash<QString, SingerInfoProvider *> m_providers;
    };

    }

    ItemSelectorListModel::ItemSelectorListModel(ListKind kind, QObject *context, const QString &contextKey,
                                                 ProjectWindowInterface *windowInterface,
                                                 QObject *parent)
            : QAbstractListModel(parent), m_kind(kind), m_context(context), m_contextKey(contextKey) {
            auto *document = windowInterface->projectDocumentContext()->document();
            m_selectionModel = document->selectionModel();
            m_musicTimeline = windowInterface->projectTimeline()->musicTimeline();

            if (m_kind == ListKind::Notes) {
                auto *clip = qobject_cast<dspx::SingingClip *>(m_context.data());
                auto *resolver = new SingerNameResolver(clip, this, true);
                m_contextSingerResolver = resolver;
                connect(resolver, &SingerNameResolver::changed, this, [this] {
                    refreshAll(rolesIncludingDescription());
                });
                m_keySignatureHelper = new KeySignatureAtSpecifiedPositionHelper(this);
                m_keySignatureHelper->setKeySignatureSequence(document->model()->keySignatures());
                connect(m_keySignatureHelper,
                        &KeySignatureAtSpecifiedPositionHelper::keySignatureLookupChanged,
                        this, [this] {
                            refreshAll({Qt::DisplayRole});
                        });
                if (clip) {
                    connect(clip, &dspx::Clip::startChanged, this, [this] {
                        refreshAll({Qt::DisplayRole});
                    });
                }
            } else if (m_kind == ListKind::DynamicAnchors) {
                auto *resolver = new SingerNameResolver(qobject_cast<dspx::SingingClip *>(m_context.data()), this);
                m_contextSingerResolver = resolver;
                connect(resolver, &SingerNameResolver::changed, this, [this] {
                    refreshAll(rolesIncludingDescription());
                });
            } else if (m_kind == ListKind::AnchorBranches) {
                m_contextParameterProvider = new ParameterInfoProvider(this);
                m_contextParameterProvider->setRegistry(CoreInterface::singerRegistry());
                m_contextParameterProvider->setArchitectureId(parameterArchitectureId());
                m_contextParameterProvider->setParameterId(m_contextKey);
            } else if (m_kind == ListKind::Anchors) {
                rebuildAnchorProvider();
            }

            if (isSelectableList()) {
                connect(m_selectionModel, &dspx::SelectionModel::selectionTypeChanged, this, [this] {
                    refreshAll({Qt::CheckStateRole});
                    Q_EMIT checkStateSummaryChanged();
                });
                connect(m_selectionModel, &dspx::SelectionModel::itemSelected, this,
                        [this](QObject *item, bool) {
                            const int row = rowForObject(item);
                            if (row >= 0) {
                                Q_EMIT dataChanged(index(row), index(row), {Qt::CheckStateRole});
                            }
                            Q_EMIT checkStateSummaryChanged();
                        });
            }
            switch (m_kind) {
                case ListKind::Clips:
                case ListKind::Labels:
                    connect(m_musicTimeline, &SVS::MusicTimeline::changed, this, [this] {
                        refreshAll(rolesIncludingDescription());
                    });
                    break;
                case ListKind::Tempos:
                case ListKind::KeySignatures:
                case ListKind::Anchors:
                case ListKind::DynamicAnchors:
                    connect(m_musicTimeline, &SVS::MusicTimeline::changed, this, [this] {
                        refreshAll(rolesIncludingDescription({Qt::DisplayRole}));
                    });
                    break;
                default:
                    break;
            }

            bindContainer();
            m_entries = sourceEntries();
            for (const auto &entry : std::as_const(m_entries)) {
                attachObject(entry);
            }
        }

    ItemSelectorListModel::~ItemSelectorListModel() {
            clearBindings();
        }

    int ItemSelectorListModel::rowCount(const QModelIndex &parent) const {
            return parent.isValid() ? 0 : m_entries.size();
        }

    QVariant ItemSelectorListModel::data(const QModelIndex &index, int role) const {
            if (!index.isValid() || index.row() < 0 || index.row() >= m_entries.size()) {
                return {};
            }
            const auto &entry = m_entries.at(index.row());
            switch (role) {
                case Qt::DisplayRole:
                    return title(entry, index.row());
                case Qt::ToolTipRole:
                case Qt::AccessibleDescriptionRole:
                case DescriptionRole:
                    return description(entry);
                case ObjectRole:
                    return QVariant::fromValue(entry.object.data());
                case NodeKindRole:
                    return static_cast<int>(entry.kind);
                case HasChildrenRole:
                    return hasChildren(entry);
                case Qt::CheckStateRole:
                    if (!isSelectable(entry)) {
                        return {};
                    }
                    return isSelected(entry) ? Qt::Checked : Qt::Unchecked;
                default:
                    return {};
            }
        }

    bool ItemSelectorListModel::setData(const QModelIndex &index, const QVariant &value, int role) {
            if (role != Qt::CheckStateRole || !index.isValid()
                || index.row() < 0 || index.row() >= m_entries.size()) {
                return false;
            }
            const auto &entry = m_entries.at(index.row());
            if (!isSelectable(entry) || !entry.object) {
                return false;
            }
            const bool selected = value.toInt() == Qt::Checked;
            if (selected == isSelected(entry)) {
                return false;
            }
            m_selectionModel->select(entry.object.data(),
                                     selected ? dspx::SelectionModel::Select
                                              : dspx::SelectionModel::Deselect);
            return true;
        }

    Qt::ItemFlags ItemSelectorListModel::flags(const QModelIndex &index) const {
            auto result = QAbstractListModel::flags(index);
            if (index.isValid() && isSelectable(m_entries.at(index.row()))) {
                result |= Qt::ItemIsUserCheckable;
            }
            return result;
        }

    Qt::CheckState ItemSelectorListModel::checkStateSummary() const {
            int selectedCount = 0;
            int selectableCount = 0;
            for (const auto &entry : m_entries) {
                if (!isSelectable(entry)) {
                    continue;
                }
                ++selectableCount;
                if (isSelected(entry)) {
                    ++selectedCount;
                }
            }
            if (selectedCount == 0) {
                return Qt::Unchecked;
            }
            return selectedCount == selectableCount ? Qt::Checked : Qt::PartiallyChecked;
        }

    bool ItemSelectorListModel::hasSelectableItems() const {
            return std::ranges::any_of(m_entries, [this](const auto &entry) {
                return isSelectable(entry);
            });
        }

    bool ItemSelectorListModel::isSelectableList() const {
            switch (m_kind) {
                case ListKind::Tracks:
                case ListKind::Clips:
                case ListKind::Notes:
                case ListKind::Anchors:
                case ListKind::DynamicAnchors:
                case ListKind::Labels:
                case ListKind::Tempos:
                case ListKind::KeySignatures:
                    return true;
                default:
                    return false;
            }
        }

    void ItemSelectorListModel::setAllSelected(bool selected) {
            const auto entries = m_entries;
            for (const auto &entry : entries) {
                if (!isSelectable(entry) || !entry.object || isSelected(entry) == selected) {
                    continue;
                }
                m_selectionModel->select(entry.object.data(),
                                         selected ? dspx::SelectionModel::Select
                                                  : dspx::SelectionModel::Deselect);
            }
        }

    const ItemSelectorEntry *ItemSelectorListModel::entryAt(int row) const {
            return row >= 0 && row < m_entries.size() ? &m_entries.at(row) : nullptr;
        }

    int ItemSelectorListModel::rowFor(NodeKind kind, QObject *object, const QString &key) const {
            for (int row = 0; row < m_entries.size(); ++row) {
                const auto &entry = m_entries.at(row);
                if (entry.kind == kind && (!object || entry.object == object)
                    && (key.isEmpty() || entry.key == key)) {
                    return row;
                }
            }
            return -1;
        }

    ListKind ItemSelectorListModel::listKind() const {
            return m_kind;
        }

    QObject *ItemSelectorListModel::context() const {
            return m_context.data();
        }

    QString ItemSelectorListModel::contextKey() const {
            return m_contextKey;
        }

    QString ItemSelectorListModel::parameterDisplayName() const {
            if (!m_contextParameterProvider) {
                return m_contextKey;
            }
            const auto displayName = m_contextParameterProvider->info().displayName;
            return displayName.isEmpty() ? m_contextKey : displayName;
        }

    bool ItemSelectorListModel::isSelectable(const ItemSelectorEntry &entry) const {
            return entry.object
                   && dspx::SelectionModel::selectionTypeFromItem(entry.object.data())
                          != dspx::SelectionModel::ST_None;
        }

    bool ItemSelectorListModel::isSelected(const ItemSelectorEntry &entry) const {
            return entry.object && m_selectionModel->isItemSelected(entry.object.data());
        }

    bool ItemSelectorListModel::hasChildren(const ItemSelectorEntry &entry) const {
            switch (entry.kind) {
                case NodeKind::RootTracks:
                case NodeKind::RootLabels:
                case NodeKind::RootTempos:
                case NodeKind::RootKeySignatures:
                case NodeKind::Track:
                case NodeKind::TrackClips:
                case NodeKind::SingingNotes:
                case NodeKind::SingingParameters:
                case NodeKind::SingingVoiceBlending:
                case NodeKind::Parameter:
                case NodeKind::FreeformEdited:
                case NodeKind::FreeformTransform:
                case NodeKind::EditedAnchors:
                case NodeKind::TransformAnchors:
                    return true;
                case NodeKind::Clip:
                    return entry.object
                           && static_cast<dspx::Clip *>(entry.object.data())->type()
                                  == dspx::Clip::Singing;
                default:
                    return false;
            }
        }

    QString ItemSelectorListModel::title(const ItemSelectorEntry &entry, int row) const {
            switch (entry.kind) {
                case NodeKind::RootTracks:
                    return tr("Tracks");
                case NodeKind::RootLabels:
                    return tr("Labels");
                case NodeKind::RootTempos:
                    return tr("Tempos");
                case NodeKind::RootKeySignatures:
                    return tr("Key Signatures");
                case NodeKind::TrackClips:
                    return tr("Clips");
                case NodeKind::SingingNotes:
                    return tr("Notes");
                case NodeKind::SingingParameters:
                    return tr("Parameters");
                case NodeKind::SingingVoiceBlending:
                    return tr("Voice Blending");
                case NodeKind::FreeformEdited:
                    return tr("Freeform Edited");
                case NodeKind::FreeformTransform:
                    return tr("Freeform Transform");
                case NodeKind::EditedAnchors:
                    return tr("Anchor Edited");
                case NodeKind::TransformAnchors:
                    return tr("Anchor Transform");
                case NodeKind::Track: {
                    const auto *track = static_cast<dspx::Track *>(entry.object.data());
                    return tr("%L1. %2").arg(row + 1).arg(track->name());
                }
                case NodeKind::Clip: {
                    const auto *clip = static_cast<dspx::Clip *>(entry.object.data());
                    const auto type = clip->type() == dspx::Clip::Audio ? tr("Audio") : tr("Singing");
                    return tr("%1: %2").arg(type, clip->name());
                }
                case NodeKind::Note: {
                    const auto *note = static_cast<dspx::Note *>(entry.object.data());
                    const auto *clip = qobject_cast<dspx::SingingClip *>(m_context.data());
                    const int position = note->position() + (clip ? clip->start() : 0);
                    const auto accidental = static_cast<SVS::MusicPitch::Accidental>(
                        m_keySignatureHelper
                            ? m_keySignatureHelper->accidentalTypeAt(position)
                            : dspx::KeySignature::Flat);
                    const auto pitch = SVS::MusicPitch(static_cast<qint8>(note->keyNumber()))
                                           .toString(accidental);
                    return tr("%1 (%2)").arg(note->lyric(), pitch);
                }
                case NodeKind::Parameter: {
                    auto *provider = m_parameterProviders.value(entry.object.data());
                    const auto displayName = provider ? provider->info().displayName : QString{};
                    return displayName.isEmpty() ? entry.key : displayName;
                }
                case NodeKind::Anchor: {
                    const auto *anchor = static_cast<dspx::AnchorNode *>(entry.object.data());
                    return tr("Position %1").arg(formatPosition(m_musicTimeline, anchor->x()));
                }
                case NodeKind::DynamicAnchor: {
                    const auto *anchor = static_cast<dspx::DynamicMixingAnchor *>(entry.object.data());
                    return tr("Position %1").arg(formatPosition(m_musicTimeline, anchor->position()));
                }
                case NodeKind::Label:
                    return static_cast<dspx::Label *>(entry.object.data())->text();
                case NodeKind::Tempo: {
                    const auto *tempo = static_cast<dspx::Tempo *>(entry.object.data());
                    const auto tempoText = tr("%L1 BPM").arg(tempo->value(), 0, 'f', 2);
                    return tr("%1 (%2)").arg(tempoText, formatPosition(m_musicTimeline, tempo->position()));
                }
                case NodeKind::KeySignature: {
                    const auto *keySignature = static_cast<dspx::KeySignature *>(entry.object.data());
                    return tr("%1 (%2)").arg(keySignatureText(keySignature),
                                             formatPosition(m_musicTimeline, keySignature->position()));
                }
            }
            return {};
        }

    QString ItemSelectorListModel::description(const ItemSelectorEntry &entry) const {
            switch (entry.kind) {
                case NodeKind::Track: {
                    const auto *track = static_cast<dspx::Track *>(entry.object.data());
                    return tr("Clip Count: %L1\nColor: %L2\nView Height: %L3\nGain: %L4 dB\nPan: %L5\nMute: %6\nSolo: %7\nRecord: %8")
                        .arg(track->clips()->size())
                        .arg(track->colorId() + 1)
                        .arg(track->height(), 0, 'f', 2)
                        .arg(SVS::DecibelLinearizer::gainToDecibels(track->gain()), 0, 'f', 1)
                        .arg(track->pan(), 0, 'f', 2)
                        .arg(yesNo(track->mute()), yesNo(track->solo()), yesNo(track->record()));
                }
                case NodeKind::Clip: {
                    const auto *clip = static_cast<dspx::Clip *>(entry.object.data());
                    auto result = tr("Position: %1\nStarting Offset: %2\nClip Length: %3")
                                      .arg(formatPosition(m_musicTimeline, clip->position()),
                                           formatOffset(clip->clipStart()), formatOffset(clip->clipLength()));
                    if (clip->type() == dspx::Clip::Singing) {
                        auto *resolver = static_cast<SingerNameResolver *>(m_singerResolvers.value(entry.object.data()));
                        result += tr("\nVirtual Singer: %1").arg(resolver ? resolver->displayName() : tr("No singer"));
                    } else {
                        const auto path = static_cast<const dspx::AudioClip *>(clip)->path();
                        result += tr("\nFile: %1").arg(QDir::toNativeSeparators(QDir(path.absoluteDir).filePath(path.fileName)));
                    }
                    return result;
                }
                case NodeKind::Note: {
                    const auto *note = static_cast<dspx::Note *>(entry.object.data());
                    auto *resolver = static_cast<SingerNameResolver *>(m_contextSingerResolver);
                    const auto language = resolver
                                              ? resolver->languageName(note->language())
                                              : note->language();
                    return tr("Onset Position (Relative to Clip): %1\nDuration: %2\nLanguage: %3\nPronunciation (Original): %4\nPronunciation (Edited): %5")
                        .arg(formatOffset(note->position()), formatOffset(note->length()), language,
                             note->originalPronunciation(), note->editedPronunciation());
                }
                case NodeKind::Parameter: {
                    const auto *parameter = static_cast<dspx::Parameter *>(entry.object.data());
                    return tr("Parameter ID: %1\nEdited Anchor Count: %L2\nTransform Anchor Count: %L3")
                        .arg(entry.key)
                        .arg(parameter->anchorEdited()->size())
                        .arg(parameter->anchorTransform()->size());
                }
                case NodeKind::Anchor: {
                    const auto *anchor = static_cast<dspx::AnchorNode *>(entry.object.data());
                    const double normalizedValue = ParameterInfo::fromDspxModelValue(anchor->y());
                    const auto value = m_anchorProvider && m_anchorProvider->exists()
                                           ? m_anchorProvider->displayString(normalizedValue)
                                           : QLocale().toString(normalizedValue);
                    return tr("Value: %1\nInterpolation: %2")
                        .arg(value, interpolationText(anchor->interpolationMode()));
                }
                case NodeKind::DynamicAnchor: {
                    const auto *anchor = static_cast<dspx::DynamicMixingAnchor *>(entry.object.data());
                    auto *resolver = static_cast<SingerNameResolver *>(m_contextSingerResolver);
                    const auto names = resolver
                                           ? resolver->dynamicSingerNames()
                                           : QStringList{};
                    const auto storedRatios = anchor->ratio();
                    QStringList lines;
                    double sum = 0.0;
                    for (int index = 0; index < names.size(); ++index) {
                        const double ratio = index + 1 == names.size()
                                                 ? std::max(0.0, 1.0 - sum)
                                                 : (index < storedRatios.size() ? storedRatios.at(index) : 0.0);
                        if (index + 1 != names.size()) {
                            sum += ratio;
                        }
                        lines.append(tr("%1: %L2%").arg(names.at(index)).arg(ratio * 100.0, 0, 'f', 1));
                    }
                    return lines.join(u'\n');
                }
                case NodeKind::Label: {
                    const auto *label = static_cast<dspx::Label *>(entry.object.data());
                    return tr("Position: %1").arg(formatPosition(m_musicTimeline, label->position()));
                }
                case NodeKind::Tempo:
                    return {};
                case NodeKind::KeySignature: {
                    const auto *keySignature = static_cast<dspx::KeySignature *>(entry.object.data());
                    auto tonality = SVS::MusicPitch(static_cast<qint8>(keySignature->tonality()))
                                        .toString(static_cast<SVS::MusicPitch::Accidental>(keySignature->accidentalType()));
                    tonality.chop(1);
                    const auto accidental = keySignature->accidentalType() == dspx::KeySignature::Flat
                                                ? tr("Flat")
                                                : tr("Sharp");
                    return tr("Position: %1\nTonality: %2\nMode: %3\nAccidental Type: %4")
                        .arg(formatPosition(m_musicTimeline, keySignature->position()), tonality,
                             musicModeName(keySignature->mode()), accidental);
                }
                default:
                    return {};
            }
        }

    QList<ItemSelectorEntry> ItemSelectorListModel::sourceEntries() const {
            QList<ItemSelectorEntry> result;
            switch (m_kind) {
                case ListKind::Root:
                    return {
                        {NodeKind::RootTracks, {}, {}},
                        {NodeKind::RootLabels, {}, {}},
                        {NodeKind::RootTempos, {}, {}},
                        {NodeKind::RootKeySignatures, {}, {}},
                    };
                case ListKind::Tracks: {
                    const auto *model = qobject_cast<dspx::Model *>(m_context.data());
                    if (model) {
                        for (auto *track : model->tracks()->items()) {
                            result.append({NodeKind::Track, track, {}});
                        }
                    }
                    break;
                }
                case ListKind::TrackBranches:
                    return {
                        {NodeKind::TrackClips, {}, {}},
                    };
                case ListKind::Clips: {
                    const auto *track = qobject_cast<dspx::Track *>(m_context.data());
                    if (track) {
                        for (auto *clip : track->clips()->asRange()) {
                            result.append({NodeKind::Clip, clip, {}});
                        }
                    }
                    break;
                }
                case ListKind::SingingBranches:
                    return {
                        {NodeKind::SingingNotes, {}, {}},
                        {NodeKind::SingingParameters, {}, {}},
                        {NodeKind::SingingVoiceBlending, {}, {}},
                    };
                case ListKind::Notes: {
                    const auto *clip = qobject_cast<dspx::SingingClip *>(m_context.data());
                    if (clip) {
                        for (auto *note : clip->notes()->asRange()) {
                            result.append({NodeKind::Note, note, {}});
                        }
                    }
                    break;
                }
                case ListKind::Parameters: {
                    const auto *clip = qobject_cast<dspx::SingingClip *>(m_context.data());
                    if (clip) {
                        const auto keys = clip->parameters()->keys();
                        for (const auto &key : keys) {
                            result.append({NodeKind::Parameter, clip->parameters()->item(key), key});
                        }
                    }
                    break;
                }
                case ListKind::AnchorBranches:
                    return {
                        {NodeKind::FreeformEdited, {}, {}},
                        {NodeKind::FreeformTransform, {}, {}},
                        {NodeKind::EditedAnchors, {}, {}},
                        {NodeKind::TransformAnchors, {}, {}},
                    };
                case ListKind::Anchors: {
                    const auto *sequence = qobject_cast<dspx::AnchorNodeSequence *>(m_context.data());
                    if (sequence) {
                        for (auto *anchor : sequence->asRange()) {
                            result.append({NodeKind::Anchor, anchor, {}});
                        }
                    }
                    break;
                }
                case ListKind::DynamicAnchors: {
                    const auto *clip = qobject_cast<dspx::SingingClip *>(m_context.data());
                    const auto *sequence = clip && clip->sources()
                                               ? clip->sources()->dynamicMixingAnchors()
                                               : nullptr;
                    if (sequence) {
                        for (auto *anchor : sequence->asRange()) {
                            result.append({NodeKind::DynamicAnchor, anchor, {}});
                        }
                    }
                    break;
                }
                case ListKind::Labels: {
                    const auto *model = qobject_cast<dspx::Model *>(m_context.data());
                    if (model) {
                        for (auto *label : model->labels()->asRange()) {
                            result.append({NodeKind::Label, label, {}});
                        }
                    }
                    break;
                }
                case ListKind::Tempos: {
                    const auto *model = qobject_cast<dspx::Model *>(m_context.data());
                    if (model) {
                        for (auto *tempo : model->tempos()->asRange()) {
                            result.append({NodeKind::Tempo, tempo, {}});
                        }
                    }
                    break;
                }
                case ListKind::KeySignatures: {
                    const auto *model = qobject_cast<dspx::Model *>(m_context.data());
                    if (model) {
                        for (auto *keySignature : model->keySignatures()->asRange()) {
                            result.append({NodeKind::KeySignature, keySignature, {}});
                        }
                    }
                    break;
                }
            }
            return result;
        }

    void ItemSelectorListModel::bindContainer() {
            switch (m_kind) {
                case ListKind::Root:
                case ListKind::TrackBranches:
                case ListKind::SingingBranches:
                    break;
                case ListKind::AnchorBranches:
                    bindParameterArchitecture();
                    break;
                case ListKind::Tracks: {
                    auto *model = qobject_cast<dspx::Model *>(m_context.data());
                    bindTrackList(model ? model->tracks() : nullptr);
                    break;
                }
                case ListKind::Clips: {
                    auto *track = qobject_cast<dspx::Track *>(m_context.data());
                    bindClipSequence(track ? track->clips() : nullptr);
                    break;
                }
                case ListKind::Notes: {
                    auto *clip = qobject_cast<dspx::SingingClip *>(m_context.data());
                    bindNoteSequence(clip ? clip->notes() : nullptr);
                    break;
                }
                case ListKind::Parameters: {
                    auto *clip = qobject_cast<dspx::SingingClip *>(m_context.data());
                    bindParameterMap(clip ? clip->parameters() : nullptr);
                    bindParameterArchitecture();
                    break;
                }
                case ListKind::Anchors:
                    bindAnchorSequence(qobject_cast<dspx::AnchorNodeSequence *>(m_context.data()));
                    bindAnchorArchitecture();
                    break;
                case ListKind::DynamicAnchors: {
                    auto *clip = qobject_cast<dspx::SingingClip *>(m_context.data());
                    if (clip) {
                        m_ownerConnections.append(connect(clip, &dspx::SingingClip::sourcesChanged,
                                                          this, &ItemSelectorListModel::switchDynamicContainer));
                    }
                    bindDynamicSequence(clip && clip->sources()
                                            ? clip->sources()->dynamicMixingAnchors()
                                            : nullptr);
                    break;
                }
                case ListKind::Labels: {
                    auto *model = qobject_cast<dspx::Model *>(m_context.data());
                    bindLabelSequence(model ? model->labels() : nullptr);
                    break;
                }
                case ListKind::Tempos: {
                    auto *model = qobject_cast<dspx::Model *>(m_context.data());
                    bindTempoSequence(model ? model->tempos() : nullptr);
                    break;
                }
                case ListKind::KeySignatures: {
                    auto *model = qobject_cast<dspx::Model *>(m_context.data());
                    bindKeySignatureSequence(model ? model->keySignatures() : nullptr);
                    break;
                }
            }
        }

    void ItemSelectorListModel::bindTrackList(dspx::TrackList *list) {
            if (!list) {
                return;
            }
            m_containerConnections.append(connect(list, &dspx::TrackList::itemInserted,
                                                  this, [this] { synchronize(); }));
            m_containerConnections.append(connect(list, &dspx::TrackList::itemRemoved,
                                                  this, [this] { synchronize(); }));
            m_containerConnections.append(connect(list, &dspx::TrackList::rotated,
                                                  this, [this] { synchronize(); }));
        }

    void ItemSelectorListModel::bindClipSequence(dspx::ClipSequence *sequence) {
            if (!sequence) {
                return;
            }
            m_containerConnections.append(connect(sequence, &dspx::ClipSequence::itemInserted,
                                                  this, [this] { synchronize(); }));
            m_containerConnections.append(connect(sequence, &dspx::ClipSequence::itemRemoved,
                                                  this, [this] { synchronize(); }));
        }

    void ItemSelectorListModel::bindNoteSequence(dspx::NoteSequence *sequence) {
            if (!sequence) {
                return;
            }
            m_containerConnections.append(connect(sequence, &dspx::NoteSequence::itemInserted,
                                                  this, [this] { synchronize(); }));
            m_containerConnections.append(connect(sequence, &dspx::NoteSequence::itemRemoved,
                                                  this, [this] { synchronize(); }));
        }

    void ItemSelectorListModel::bindParameterMap(dspx::ParameterMap *map) {
            if (!map) {
                return;
            }
            m_containerConnections.append(connect(map, &dspx::ParameterMap::itemInserted,
                                                  this, [this] { synchronize(); }));
            m_containerConnections.append(connect(map, &dspx::ParameterMap::itemRemoved,
                                                  this, [this] { synchronize(); }));
        }

    void ItemSelectorListModel::bindAnchorSequence(dspx::AnchorNodeSequence *sequence) {
            if (!sequence) {
                return;
            }
            m_containerConnections.append(connect(sequence, &dspx::AnchorNodeSequence::itemInserted,
                                                  this, [this] { synchronize(); }));
            m_containerConnections.append(connect(sequence, &dspx::AnchorNodeSequence::itemRemoved,
                                                  this, [this] { synchronize(); }));
        }

    void ItemSelectorListModel::bindDynamicSequence(dspx::DynamicMixingAnchorSequence *sequence) {
            if (!sequence) {
                return;
            }
            m_containerConnections.append(connect(sequence, &dspx::DynamicMixingAnchorSequence::itemInserted,
                                                  this, [this] { synchronize(); }));
            m_containerConnections.append(connect(sequence, &dspx::DynamicMixingAnchorSequence::itemRemoved,
                                                  this, [this] { synchronize(); }));
        }

    void ItemSelectorListModel::bindLabelSequence(dspx::LabelSequence *sequence) {
            if (!sequence) {
                return;
            }
            m_containerConnections.append(connect(sequence, &dspx::LabelSequence::itemInserted,
                                                  this, [this] { synchronize(); }));
            m_containerConnections.append(connect(sequence, &dspx::LabelSequence::itemRemoved,
                                                  this, [this] { synchronize(); }));
        }

    void ItemSelectorListModel::bindTempoSequence(dspx::TempoSequence *sequence) {
            if (!sequence) {
                return;
            }
            m_containerConnections.append(connect(sequence, &dspx::TempoSequence::itemInserted,
                                                  this, [this] { synchronize(); }));
            m_containerConnections.append(connect(sequence, &dspx::TempoSequence::itemRemoved,
                                                  this, [this] { synchronize(); }));
        }

    void ItemSelectorListModel::bindKeySignatureSequence(dspx::KeySignatureSequence *sequence) {
            if (!sequence) {
                return;
            }
            m_containerConnections.append(connect(sequence, &dspx::KeySignatureSequence::itemInserted,
                                                  this, [this] { synchronize(); }));
            m_containerConnections.append(connect(sequence, &dspx::KeySignatureSequence::itemRemoved,
                                                  this, [this] { synchronize(); }));
        }

    void ItemSelectorListModel::switchDynamicContainer() {
            for (const auto &connection : std::as_const(m_containerConnections)) {
                disconnect(connection);
            }
            m_containerConnections.clear();

            beginResetModel();
            clearObjectBindings();
            auto *clip = qobject_cast<dspx::SingingClip *>(m_context.data());
            bindDynamicSequence(clip && clip->sources()
                                    ? clip->sources()->dynamicMixingAnchors()
                                    : nullptr);
            m_entries = sourceEntries();
            for (const auto &entry : std::as_const(m_entries)) {
                attachObject(entry);
            }
            endResetModel();
            Q_EMIT checkStateSummaryChanged();
        }

    void ItemSelectorListModel::synchronize() {
            const auto desired = sourceEntries();
            bool changed = false;

            for (int row = m_entries.size() - 1; row >= 0; --row) {
                const auto found = std::ranges::any_of(desired, [this, row](const auto &entry) {
                    return m_entries.at(row).matches(entry);
                });
                if (found) {
                    continue;
                }
                const auto removed = m_entries.at(row);
                beginRemoveRows({}, row, row);
                m_entries.removeAt(row);
                endRemoveRows();
                detachObject(removed.object);
                changed = true;
            }

            for (int targetRow = 0; targetRow < desired.size(); ++targetRow) {
                if (targetRow < m_entries.size() && m_entries.at(targetRow).matches(desired.at(targetRow))) {
                    continue;
                }

                int sourceRow = -1;
                for (int row = targetRow + 1; row < m_entries.size(); ++row) {
                    if (m_entries.at(row).matches(desired.at(targetRow))) {
                        sourceRow = row;
                        break;
                    }
                }
                if (sourceRow >= 0) {
                    beginMoveRows({}, sourceRow, sourceRow, {}, targetRow);
                    m_entries.move(sourceRow, targetRow);
                    endMoveRows();
                } else {
                    beginInsertRows({}, targetRow, targetRow);
                    m_entries.insert(targetRow, desired.at(targetRow));
                    endInsertRows();
                    attachObject(desired.at(targetRow));
                }
                changed = true;
            }

            if (changed) {
                refreshAll(rolesIncludingDescription({Qt::DisplayRole, Qt::CheckStateRole}));
                Q_EMIT checkStateSummaryChanged();
            }
        }

    void ItemSelectorListModel::attachObject(const ItemSelectorEntry &entry) {
            auto *object = entry.object.data();
            if (!object || m_objectConnections.contains(object)) {
                return;
            }

            const auto refresh = [this, object] {
                refreshObject(object, false);
            };
            const auto refreshAndSort = [this, object] {
                refreshObject(object, true);
            };

            QList<QMetaObject::Connection> connections;
            switch (entry.kind) {
                case NodeKind::Track: {
                    auto *track = static_cast<dspx::Track *>(object);
                    connections.append(connect(track, &dspx::Track::nameChanged, this, refresh));
                    connections.append(connect(track, &dspx::Track::colorIdChanged, this, refresh));
                    connections.append(connect(track, &dspx::Track::heightChanged, this, refresh));
                    connections.append(connect(track, &dspx::Track::gainChanged, this, refresh));
                    connections.append(connect(track, &dspx::Track::panChanged, this, refresh));
                    connections.append(connect(track, &dspx::Track::muteChanged, this, refresh));
                    connections.append(connect(track, &dspx::Track::soloChanged, this, refresh));
                    connections.append(connect(track, &dspx::Track::recordChanged, this, refresh));
                    connections.append(connect(track->clips(), &dspx::ClipSequence::sizeChanged, this, refresh));
                    break;
                }
                case NodeKind::Clip: {
                    auto *clip = static_cast<dspx::Clip *>(object);
                    connections.append(connect(clip, &dspx::Clip::nameChanged, this, refresh));
                    connections.append(connect(clip, &dspx::Clip::positionChanged, this, refreshAndSort));
                    connections.append(connect(clip, &dspx::Clip::clipStartChanged, this, refresh));
                    connections.append(connect(clip, &dspx::Clip::clipLengthChanged, this, refresh));
                    if (clip->type() == dspx::Clip::Singing) {
                        auto *resolver = new SingerNameResolver(static_cast<dspx::SingingClip *>(clip), this);
                        connections.append(connect(resolver, &SingerNameResolver::changed, this, refresh));
                        m_singerResolvers.insert(object, resolver);
                    } else {
                        connections.append(connect(static_cast<dspx::AudioClip *>(clip),
                                                   &dspx::AudioClip::pathChanged, this, refresh));
                    }
                    break;
                }
                case NodeKind::Note: {
                    auto *note = static_cast<dspx::Note *>(object);
                    connections.append(connect(note, &dspx::Note::lyricChanged, this, refresh));
                    connections.append(connect(note, &dspx::Note::keyNumberChanged, this, refresh));
                    connections.append(connect(note, &dspx::Note::languageChanged, this, refresh));
                    connections.append(connect(note, &dspx::Note::positionChanged, this, refreshAndSort));
                    connections.append(connect(note, &dspx::Note::lengthChanged, this, refresh));
                    connections.append(connect(note, &dspx::Note::originalPronunciationChanged, this, refresh));
                    connections.append(connect(note, &dspx::Note::editedPronunciationChanged, this, refresh));
                    break;
                }
                case NodeKind::Parameter: {
                    auto *parameter = static_cast<dspx::Parameter *>(object);
                    connections.append(connect(parameter->anchorEdited(), &dspx::AnchorNodeSequence::sizeChanged,
                                               this, refresh));
                    connections.append(connect(parameter->anchorTransform(), &dspx::AnchorNodeSequence::sizeChanged,
                                               this, refresh));
                    auto *provider = new ParameterInfoProvider(this);
                    provider->setRegistry(CoreInterface::singerRegistry());
                    provider->setArchitectureId(parameterArchitectureId());
                    provider->setParameterId(entry.key);
                    connections.append(connect(provider, &ParameterInfoProvider::infoChanged, this, refresh));
                    connections.append(connect(provider, &ParameterInfoProvider::existsChanged, this, refresh));
                    m_parameterProviders.insert(object, provider);
                    break;
                }
                case NodeKind::Anchor: {
                    auto *anchor = static_cast<dspx::AnchorNode *>(object);
                    connections.append(connect(anchor, &dspx::AnchorNode::xChanged, this, refreshAndSort));
                    connections.append(connect(anchor, &dspx::AnchorNode::yChanged, this, refresh));
                    connections.append(connect(anchor, &dspx::AnchorNode::interpolationModeChanged, this, refresh));
                    break;
                }
                case NodeKind::DynamicAnchor: {
                    auto *anchor = static_cast<dspx::DynamicMixingAnchor *>(object);
                    connections.append(connect(anchor, &dspx::DynamicMixingAnchor::positionChanged,
                                               this, refreshAndSort));
                    connections.append(connect(anchor, &dspx::DynamicMixingAnchor::ratioChanged, this, refresh));
                    break;
                }
                case NodeKind::Label: {
                    auto *label = static_cast<dspx::Label *>(object);
                    connections.append(connect(label, &dspx::Label::positionChanged, this, refreshAndSort));
                    connections.append(connect(label, &dspx::Label::textChanged, this, refresh));
                    break;
                }
                case NodeKind::Tempo: {
                    auto *tempo = static_cast<dspx::Tempo *>(object);
                    connections.append(connect(tempo, &dspx::Tempo::positionChanged, this, refreshAndSort));
                    connections.append(connect(tempo, &dspx::Tempo::valueChanged, this, refresh));
                    break;
                }
                case NodeKind::KeySignature: {
                    auto *keySignature = static_cast<dspx::KeySignature *>(object);
                    connections.append(connect(keySignature, &dspx::KeySignature::positionChanged,
                                               this, refreshAndSort));
                    connections.append(connect(keySignature, &dspx::KeySignature::modeChanged, this, refresh));
                    connections.append(connect(keySignature, &dspx::KeySignature::tonalityChanged, this, refresh));
                    connections.append(connect(keySignature, &dspx::KeySignature::accidentalTypeChanged,
                                               this, refresh));
                    break;
                }
                default:
                    break;
            }
            m_objectConnections.insert(object, connections);
        }

    void ItemSelectorListModel::detachObject(QObject *object) {
            if (!object) {
                return;
            }
            const auto connections = m_objectConnections.take(object);
            for (const auto &connection : connections) {
                disconnect(connection);
            }
            delete m_singerResolvers.take(object);
            delete m_parameterProviders.take(object);
        }

    void ItemSelectorListModel::refreshObject(QObject *object, bool reorder) {
            if (reorder) {
                synchronize();
            }
            const int row = rowForObject(object);
            if (row >= 0) {
                Q_EMIT dataChanged(index(row), index(row),
                                   rolesIncludingDescription({Qt::DisplayRole}));
            }
        }

    int ItemSelectorListModel::rowForObject(QObject *object) const {
            for (int row = 0; row < m_entries.size(); ++row) {
                if (m_entries.at(row).object == object) {
                    return row;
                }
            }
            return -1;
        }

    void ItemSelectorListModel::refreshAll(const QList<int> &roles) {
            if (!m_entries.isEmpty()) {
                Q_EMIT dataChanged(index(0), index(m_entries.size() - 1), roles);
            }
        }

    dspx::SingingClip *ItemSelectorListModel::parameterClip() const {
            if (m_kind == ListKind::Parameters) {
                return qobject_cast<dspx::SingingClip *>(m_context.data());
            }
            if (m_kind == ListKind::AnchorBranches) {
                auto *parameter = qobject_cast<dspx::Parameter *>(m_context.data());
                return parameter && parameter->parameterMap()
                           ? parameter->parameterMap()->singingClip()
                           : nullptr;
            }
            if (m_kind == ListKind::Anchors) {
                auto *sequence = qobject_cast<dspx::AnchorNodeSequence *>(m_context.data());
                return sequence && sequence->parameter() && sequence->parameter()->parameterMap()
                           ? sequence->parameter()->parameterMap()->singingClip()
                           : nullptr;
            }
            return nullptr;
        }

    QString ItemSelectorListModel::parameterArchitectureId() const {
            auto *clip = parameterClip();
            return clip && clip->sources() ? clip->sources()->category() : QString{};
        }

    void ItemSelectorListModel::bindParameterArchitecture() {
            clearMetadataConnections();
            auto *clip = parameterClip();
            if (!clip) {
                return;
            }
            m_metadataConnections.append(connect(clip, &dspx::SingingClip::sourcesChanged, this, [this] {
                bindParameterArchitecture();
                updateParameterProviders();
            }));
            if (clip->sources()) {
                m_metadataConnections.append(connect(clip->sources(), &dspx::Sources::categoryChanged,
                                                     this, [this] { updateParameterProviders(); }));
            }
        }

    void ItemSelectorListModel::updateParameterProviders() {
            const auto architectureId = parameterArchitectureId();
            for (auto *provider : std::as_const(m_parameterProviders)) {
                provider->setArchitectureId(architectureId);
            }
            if (m_contextParameterProvider) {
                m_contextParameterProvider->setArchitectureId(architectureId);
            }
        }

    void ItemSelectorListModel::rebuildAnchorProvider() {
            auto *sequence = qobject_cast<dspx::AnchorNodeSequence *>(m_context.data());
            if (!sequence || !sequence->parameter() || !sequence->parameter()->parameterMap()) {
                return;
            }

            m_anchorProvider = new ParameterInfoProvider(this);
            m_anchorProvider->setRegistry(CoreInterface::singerRegistry());
            m_anchorProvider->setArchitectureId(parameterArchitectureId());
            m_anchorProvider->setParameterId(m_contextKey);
            m_anchorProvider->setTransform(sequence->role() == dspx::AnchorNodeSequence::Transform);
            connect(m_anchorProvider, &ParameterInfoProvider::infoChanged, this, [this] {
                refreshAll(rolesIncludingDescription());
            });
            connect(m_anchorProvider, &ParameterInfoProvider::existsChanged, this, [this] {
                refreshAll(rolesIncludingDescription());
            });
        }

    void ItemSelectorListModel::bindAnchorArchitecture() {
            clearMetadataConnections();
            auto *sequence = qobject_cast<dspx::AnchorNodeSequence *>(m_context.data());
            auto *clip = sequence && sequence->parameter() && sequence->parameter()->parameterMap()
                             ? sequence->parameter()->parameterMap()->singingClip()
                             : nullptr;
            if (!clip) {
                return;
            }
            m_metadataConnections.append(connect(clip, &dspx::SingingClip::sourcesChanged, this, [this] {
                bindAnchorArchitecture();
                if (m_anchorProvider) {
                    m_anchorProvider->setArchitectureId(parameterArchitectureId());
                }
            }));
            if (clip->sources()) {
                m_metadataConnections.append(connect(clip->sources(), &dspx::Sources::categoryChanged,
                                                     this, [this] {
                                                         if (m_anchorProvider) {
                                                             m_anchorProvider->setArchitectureId(parameterArchitectureId());
                                                         }
                                                     }));
            }
        }

    void ItemSelectorListModel::clearObjectBindings() {
            for (const auto &connections : std::as_const(m_objectConnections)) {
                for (const auto &connection : connections) {
                    disconnect(connection);
                }
            }
            m_objectConnections.clear();
            qDeleteAll(m_singerResolvers);
            m_singerResolvers.clear();
            qDeleteAll(m_parameterProviders);
            m_parameterProviders.clear();
        }

    void ItemSelectorListModel::clearMetadataConnections() {
            for (const auto &connection : std::as_const(m_metadataConnections)) {
                disconnect(connection);
            }
            m_metadataConnections.clear();
        }

    void ItemSelectorListModel::clearBindings() {
            for (const auto &connection : std::as_const(m_ownerConnections)) {
                disconnect(connection);
            }
            for (const auto &connection : std::as_const(m_containerConnections)) {
                disconnect(connection);
            }
            clearMetadataConnections();
            m_ownerConnections.clear();
            m_containerConnections.clear();
            clearObjectBindings();
        }

}

#include "ItemSelectorListModel.moc"
