// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#include "ItemSelectorDialog.h"

#include <algorithm>
#include <limits>
#include <utility>

#include <QAbstractListModel>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QFrame>
#include <QHBoxLayout>
#include <QHash>
#include <QHeaderView>
#include <QHideEvent>
#include <QItemSelectionModel>
#include <QKeyEvent>
#include <QLocale>
#include <QMap>
#include <QMetaType>
#include <QMouseEvent>
#include <QPointer>
#include <QScrollArea>
#include <QScrollBar>
#include <QSignalBlocker>
#include <QShowEvent>
#include <QSizePolicy>
#include <QStyle>
#include <QStyleOptionViewItem>
#include <QTimer>
#include <QTreeView>
#include <QVBoxLayout>

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
#include <coreplugin/internal/KeySignatureAtSpecifiedPositionHelper.h>

namespace Core::Internal {

    enum class NodeKind {
        RootTracks,
        RootLabels,
        RootTempos,
        RootKeySignatures,
        Track,
        Clip,
        SingingNotes,
        SingingParameters,
        SingingVoiceBlending,
        Note,
        Parameter,
        EditedAnchors,
        TransformAnchors,
        Anchor,
        DynamicAnchor,
        Label,
        Tempo,
        KeySignature,
    };

    enum class ListKind {
        Root,
        Tracks,
        Clips,
        SingingBranches,
        Notes,
        Parameters,
        AnchorBranches,
        Anchors,
        DynamicAnchors,
        Labels,
        Tempos,
        KeySignatures,
    };

    struct ItemSelectorEntry {
        NodeKind kind{};
        QPointer<QObject> object;
        QString key;

        bool matches(const ItemSelectorEntry &other) const {
            return kind == other.kind && object == other.object && key == other.key;
        }
    };

    static QString yesNo(bool value) {
        return value ? ItemSelectorDialog::tr("Yes") : ItemSelectorDialog::tr("No");
    }

    static QString formatOffset(int tick) {
        return SVS::MusicTimeOffset(tick).toString(1, 3);
    }

    static QString formatPosition(SVS::MusicTimeline *timeline, int tick) {
        return timeline ? timeline->create(0, 0, tick).toString(1, 1, 3) : QString{};
    }

    static QString interpolationText(dspx::AnchorNode::InterpolationMode mode) {
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

    static QString musicModeName(int mode) {
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

    static QString keySignatureText(const dspx::KeySignature *keySignature) {
        const auto modeName = musicModeName(keySignature->mode());
        if (keySignature->mode() == 0) {
            return modeName;
        }
        auto keyName = SVS::MusicPitch(static_cast<qint8>(keySignature->tonality()))
                           .toString(static_cast<SVS::MusicPitch::Accidental>(keySignature->accidentalType()));
        keyName.chop(1);
        return QStringLiteral("%1 %2").arg(keyName, modeName);
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

        static QString firstLeafPath(const QVariantList &tree, const QString &parentPath) {
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

        QPointer<dspx::SingingClip> m_clip;
        ClipSingerIdProvider *m_clipProvider;
        QHash<QString, SingerInfoProvider *> m_providers;
    };

    class ItemSelectorView : public QTreeView {
        Q_OBJECT

    public:
        explicit ItemSelectorView(QWidget *parent = nullptr) : QTreeView(parent) {
        }

    Q_SIGNALS:
        void focusPreviousRequested();
        void focusNextRequested();

    protected:
        void mousePressEvent(QMouseEvent *event) override {
            const auto index = indexAt(event->position().toPoint());
            if (event->button() == Qt::LeftButton && index.isValid()
                && index.flags().testFlag(Qt::ItemIsUserCheckable)
                && checkIndicatorRect(index).contains(event->position().toPoint())) {
                toggleCheckState(index);
                m_checkboxPressed = true;
                event->accept();
                return;
            }
            QTreeView::mousePressEvent(event);
        }

        void mouseReleaseEvent(QMouseEvent *event) override {
            if (m_checkboxPressed) {
                m_checkboxPressed = false;
                event->accept();
                return;
            }
            QTreeView::mouseReleaseEvent(event);
        }

        void mouseDoubleClickEvent(QMouseEvent *event) override {
            const auto index = indexAt(event->position().toPoint());
            if (event->button() == Qt::LeftButton && index.isValid()
                && index.flags().testFlag(Qt::ItemIsUserCheckable)
                && checkIndicatorRect(index).contains(event->position().toPoint())) {
                m_checkboxPressed = true;
                event->accept();
                return;
            }
            QTreeView::mouseDoubleClickEvent(event);
        }

        void keyPressEvent(QKeyEvent *event) override {
            if (event->key() == Qt::Key_Space && currentIndex().isValid()
                && currentIndex().flags().testFlag(Qt::ItemIsUserCheckable)) {
                toggleCheckState(currentIndex());
                event->accept();
                return;
            }
            if (event->key() == Qt::Key_Left) {
                if (isRightToLeft()) {
                    Q_EMIT focusNextRequested();
                } else {
                    Q_EMIT focusPreviousRequested();
                }
                event->accept();
                return;
            }
            if (event->key() == Qt::Key_Right) {
                if (isRightToLeft()) {
                    Q_EMIT focusPreviousRequested();
                } else {
                    Q_EMIT focusNextRequested();
                }
                event->accept();
                return;
            }
            QTreeView::keyPressEvent(event);
        }

    private:
        QRect checkIndicatorRect(const QModelIndex &index) const {
            QStyleOptionViewItem option;
            option.initFrom(this);
            option.index = index;
            option.rect = visualRect(index);
            option.features |= QStyleOptionViewItem::HasCheckIndicator;
            option.checkState = static_cast<Qt::CheckState>(index.data(Qt::CheckStateRole).toInt());
            return style()->subElementRect(QStyle::SE_ItemViewItemCheckIndicator, &option, this);
        }

        static void toggleCheckState(const QModelIndex &index) {
            const auto current = static_cast<Qt::CheckState>(index.data(Qt::CheckStateRole).toInt());
            const auto next = current == Qt::Checked ? Qt::Unchecked : Qt::Checked;
            const_cast<QAbstractItemModel *>(index.model())->setData(index, next, Qt::CheckStateRole);
        }

        bool m_checkboxPressed = false;
    };

    class SelectAllCheckBox : public QCheckBox {
    public:
        explicit SelectAllCheckBox(QWidget *parent = nullptr) : QCheckBox(parent) {
            setTristate(true);
        }

    protected:
        void nextCheckState() override {
            setCheckState(checkState() == Qt::Checked ? Qt::Unchecked : Qt::Checked);
        }
    };

    class ItemSelectorListModel : public QAbstractListModel {
        Q_OBJECT

    public:
        enum Role {
            DescriptionRole = Qt::UserRole + 1,
            ObjectRole,
            NodeKindRole,
            HasChildrenRole,
        };

        ItemSelectorListModel(ListKind kind, QObject *context, ProjectWindowInterface *windowInterface,
                              QObject *parent = nullptr)
            : QAbstractListModel(parent), m_kind(kind), m_context(context) {
            auto *document = windowInterface->projectDocumentContext()->document();
            m_selectionModel = document->selectionModel();
            m_musicTimeline = windowInterface->projectTimeline()->musicTimeline();

            if (m_kind == ListKind::Notes) {
                auto *clip = qobject_cast<dspx::SingingClip *>(m_context.data());
                m_contextSingerResolver = new SingerNameResolver(
                    clip, this, true);
                connect(m_contextSingerResolver, &SingerNameResolver::changed, this, [this] {
                    refreshAll({DescriptionRole});
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
                m_contextSingerResolver = new SingerNameResolver(qobject_cast<dspx::SingingClip *>(m_context.data()), this);
                connect(m_contextSingerResolver, &SingerNameResolver::changed, this, [this] {
                    refreshAll({DescriptionRole});
                });
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
                case ListKind::Tempos:
                case ListKind::KeySignatures:
                    connect(m_musicTimeline, &SVS::MusicTimeline::changed, this, [this] {
                        refreshAll({DescriptionRole});
                    });
                    break;
                case ListKind::Anchors:
                case ListKind::DynamicAnchors:
                    connect(m_musicTimeline, &SVS::MusicTimeline::changed, this, [this] {
                        refreshAll({Qt::DisplayRole, DescriptionRole});
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

        ~ItemSelectorListModel() override {
            clearBindings();
        }

        int rowCount(const QModelIndex &parent = {}) const override {
            return parent.isValid() ? 0 : m_entries.size();
        }

        QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override {
            if (!index.isValid() || index.row() < 0 || index.row() >= m_entries.size()) {
                return {};
            }
            const auto &entry = m_entries.at(index.row());
            switch (role) {
                case Qt::DisplayRole:
                    return title(entry, index.row());
                case Qt::ToolTipRole:
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

        bool setData(const QModelIndex &index, const QVariant &value, int role) override {
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

        Qt::ItemFlags flags(const QModelIndex &index) const override {
            auto result = QAbstractListModel::flags(index);
            if (index.isValid() && isSelectable(m_entries.at(index.row()))) {
                result |= Qt::ItemIsUserCheckable;
            }
            return result;
        }

        Qt::CheckState checkStateSummary() const {
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

        bool hasSelectableItems() const {
            return std::ranges::any_of(m_entries, [this](const auto &entry) {
                return isSelectable(entry);
            });
        }

        bool isSelectableList() const {
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

        void setAllSelected(bool selected) {
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

        const ItemSelectorEntry *entryAt(int row) const {
            return row >= 0 && row < m_entries.size() ? &m_entries.at(row) : nullptr;
        }

        int rowFor(NodeKind kind, QObject *object = nullptr, const QString &key = {}) const {
            for (int row = 0; row < m_entries.size(); ++row) {
                const auto &entry = m_entries.at(row);
                if (entry.kind == kind && (!object || entry.object == object)
                    && (key.isEmpty() || entry.key == key)) {
                    return row;
                }
            }
            return -1;
        }

        ListKind listKind() const {
            return m_kind;
        }

        QObject *context() const {
            return m_context.data();
        }

    Q_SIGNALS:
        void checkStateSummaryChanged();

    private:
        static bool isSelectable(const ItemSelectorEntry &entry) {
            return entry.object
                   && dspx::SelectionModel::selectionTypeFromItem(entry.object.data())
                          != dspx::SelectionModel::ST_None;
        }

        bool isSelected(const ItemSelectorEntry &entry) const {
            return entry.object && m_selectionModel->isItemSelected(entry.object.data());
        }

        static bool hasChildren(const ItemSelectorEntry &entry) {
            switch (entry.kind) {
                case NodeKind::RootTracks:
                case NodeKind::RootLabels:
                case NodeKind::RootTempos:
                case NodeKind::RootKeySignatures:
                case NodeKind::Track:
                case NodeKind::SingingNotes:
                case NodeKind::SingingParameters:
                case NodeKind::SingingVoiceBlending:
                case NodeKind::Parameter:
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

        QString title(const ItemSelectorEntry &entry, int row) const {
            switch (entry.kind) {
                case NodeKind::RootTracks:
                    return tr("Tracks");
                case NodeKind::RootLabels:
                    return tr("Labels");
                case NodeKind::RootTempos:
                    return tr("Tempos");
                case NodeKind::RootKeySignatures:
                    return tr("Key Signatures");
                case NodeKind::SingingNotes:
                    return tr("Notes");
                case NodeKind::SingingParameters:
                    return tr("Parameters");
                case NodeKind::SingingVoiceBlending:
                    return tr("Voice Blending");
                case NodeKind::EditedAnchors:
                    return tr("Edited Anchors");
                case NodeKind::TransformAnchors:
                    return tr("Transform Anchors");
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
                case NodeKind::Tempo:
                    return tr("%L1 BPM").arg(static_cast<dspx::Tempo *>(entry.object.data())->value(), 0, 'f', 2);
                case NodeKind::KeySignature:
                    return keySignatureText(static_cast<dspx::KeySignature *>(entry.object.data()));
            }
            return {};
        }

        QString description(const ItemSelectorEntry &entry) const {
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
                        auto *resolver = m_singerResolvers.value(entry.object.data());
                        result += tr("\nVirtual Singer: %1").arg(resolver ? resolver->displayName() : tr("No singer"));
                    } else {
                        const auto path = static_cast<const dspx::AudioClip *>(clip)->path();
                        result += tr("\nFile: %1").arg(QDir::toNativeSeparators(QDir(path.absoluteDir).filePath(path.fileName)));
                    }
                    return result;
                }
                case NodeKind::Note: {
                    const auto *note = static_cast<dspx::Note *>(entry.object.data());
                    const auto language = m_contextSingerResolver
                                              ? m_contextSingerResolver->languageName(note->language())
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
                    const auto value = m_anchorProvider && m_anchorProvider->exists()
                                           ? m_anchorProvider->displayString(anchor->y())
                                           : QLocale().toString(anchor->y());
                    return tr("Value: %1\nInterpolation: %2")
                        .arg(value, interpolationText(anchor->interpolationMode()));
                }
                case NodeKind::DynamicAnchor: {
                    const auto *anchor = static_cast<dspx::DynamicMixingAnchor *>(entry.object.data());
                    const auto names = m_contextSingerResolver
                                           ? m_contextSingerResolver->dynamicSingerNames()
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
                case NodeKind::Tempo: {
                    const auto *tempo = static_cast<dspx::Tempo *>(entry.object.data());
                    return tr("Position: %1").arg(formatPosition(m_musicTimeline, tempo->position()));
                }
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

        QList<ItemSelectorEntry> sourceEntries() const {
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

        void bindContainer() {
            switch (m_kind) {
                case ListKind::Root:
                case ListKind::SingingBranches:
                case ListKind::AnchorBranches:
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

        void bindTrackList(dspx::TrackList *list) {
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

        void bindClipSequence(dspx::ClipSequence *sequence) {
            if (!sequence) {
                return;
            }
            m_containerConnections.append(connect(sequence, &dspx::ClipSequence::itemInserted,
                                                  this, [this] { synchronize(); }));
            m_containerConnections.append(connect(sequence, &dspx::ClipSequence::itemRemoved,
                                                  this, [this] { synchronize(); }));
        }

        void bindNoteSequence(dspx::NoteSequence *sequence) {
            if (!sequence) {
                return;
            }
            m_containerConnections.append(connect(sequence, &dspx::NoteSequence::itemInserted,
                                                  this, [this] { synchronize(); }));
            m_containerConnections.append(connect(sequence, &dspx::NoteSequence::itemRemoved,
                                                  this, [this] { synchronize(); }));
        }

        void bindParameterMap(dspx::ParameterMap *map) {
            if (!map) {
                return;
            }
            m_containerConnections.append(connect(map, &dspx::ParameterMap::itemInserted,
                                                  this, [this] { synchronize(); }));
            m_containerConnections.append(connect(map, &dspx::ParameterMap::itemRemoved,
                                                  this, [this] { synchronize(); }));
        }

        void bindAnchorSequence(dspx::AnchorNodeSequence *sequence) {
            if (!sequence) {
                return;
            }
            m_containerConnections.append(connect(sequence, &dspx::AnchorNodeSequence::itemInserted,
                                                  this, [this] { synchronize(); }));
            m_containerConnections.append(connect(sequence, &dspx::AnchorNodeSequence::itemRemoved,
                                                  this, [this] { synchronize(); }));
        }

        void bindDynamicSequence(dspx::DynamicMixingAnchorSequence *sequence) {
            if (!sequence) {
                return;
            }
            m_containerConnections.append(connect(sequence, &dspx::DynamicMixingAnchorSequence::itemInserted,
                                                  this, [this] { synchronize(); }));
            m_containerConnections.append(connect(sequence, &dspx::DynamicMixingAnchorSequence::itemRemoved,
                                                  this, [this] { synchronize(); }));
        }

        void bindLabelSequence(dspx::LabelSequence *sequence) {
            if (!sequence) {
                return;
            }
            m_containerConnections.append(connect(sequence, &dspx::LabelSequence::itemInserted,
                                                  this, [this] { synchronize(); }));
            m_containerConnections.append(connect(sequence, &dspx::LabelSequence::itemRemoved,
                                                  this, [this] { synchronize(); }));
        }

        void bindTempoSequence(dspx::TempoSequence *sequence) {
            if (!sequence) {
                return;
            }
            m_containerConnections.append(connect(sequence, &dspx::TempoSequence::itemInserted,
                                                  this, [this] { synchronize(); }));
            m_containerConnections.append(connect(sequence, &dspx::TempoSequence::itemRemoved,
                                                  this, [this] { synchronize(); }));
        }

        void bindKeySignatureSequence(dspx::KeySignatureSequence *sequence) {
            if (!sequence) {
                return;
            }
            m_containerConnections.append(connect(sequence, &dspx::KeySignatureSequence::itemInserted,
                                                  this, [this] { synchronize(); }));
            m_containerConnections.append(connect(sequence, &dspx::KeySignatureSequence::itemRemoved,
                                                  this, [this] { synchronize(); }));
        }

        void switchDynamicContainer() {
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

        void synchronize() {
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
                refreshAll({Qt::DisplayRole, DescriptionRole, Qt::CheckStateRole});
                Q_EMIT checkStateSummaryChanged();
            }
        }

        void attachObject(const ItemSelectorEntry &entry) {
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

        void detachObject(QObject *object) {
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

        void refreshObject(QObject *object, bool reorder) {
            if (reorder) {
                synchronize();
            }
            const int row = rowForObject(object);
            if (row >= 0) {
                Q_EMIT dataChanged(index(row), index(row), {Qt::DisplayRole, DescriptionRole});
            }
        }

        int rowForObject(QObject *object) const {
            for (int row = 0; row < m_entries.size(); ++row) {
                if (m_entries.at(row).object == object) {
                    return row;
                }
            }
            return -1;
        }

        void refreshAll(const QList<int> &roles) {
            if (!m_entries.isEmpty()) {
                Q_EMIT dataChanged(index(0), index(m_entries.size() - 1), roles);
            }
        }

        QString parameterArchitectureId() const {
            auto *clip = qobject_cast<dspx::SingingClip *>(m_context.data());
            if (m_kind == ListKind::Anchors) {
                auto *sequence = qobject_cast<dspx::AnchorNodeSequence *>(m_context.data());
                clip = sequence && sequence->parameter() && sequence->parameter()->parameterMap()
                           ? sequence->parameter()->parameterMap()->singingClip()
                           : nullptr;
            }
            return clip && clip->sources() ? clip->sources()->category() : QString{};
        }

        void bindParameterArchitecture() {
            clearMetadataConnections();
            auto *clip = qobject_cast<dspx::SingingClip *>(m_context.data());
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

        void updateParameterProviders() {
            const auto architectureId = parameterArchitectureId();
            for (auto *provider : std::as_const(m_parameterProviders)) {
                provider->setArchitectureId(architectureId);
            }
        }

        void rebuildAnchorProvider() {
            auto *sequence = qobject_cast<dspx::AnchorNodeSequence *>(m_context.data());
            if (!sequence || !sequence->parameter() || !sequence->parameter()->parameterMap()) {
                return;
            }
            const auto *map = sequence->parameter()->parameterMap();
            const auto keys = map->keys();
            const auto items = map->items();
            const int parameterIndex = items.indexOf(sequence->parameter());
            const auto parameterId = parameterIndex >= 0 && parameterIndex < keys.size()
                                         ? keys.at(parameterIndex)
                                         : QString{};

            m_anchorProvider = new ParameterInfoProvider(this);
            m_anchorProvider->setRegistry(CoreInterface::singerRegistry());
            m_anchorProvider->setArchitectureId(parameterArchitectureId());
            m_anchorProvider->setParameterId(parameterId);
            m_anchorProvider->setTransform(sequence->role() == dspx::AnchorNodeSequence::Transform);
            connect(m_anchorProvider, &ParameterInfoProvider::infoChanged, this, [this] {
                refreshAll({DescriptionRole});
            });
            connect(m_anchorProvider, &ParameterInfoProvider::existsChanged, this, [this] {
                refreshAll({DescriptionRole});
            });
        }

        void bindAnchorArchitecture() {
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

        void clearObjectBindings() {
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

        void clearMetadataConnections() {
            for (const auto &connection : std::as_const(m_metadataConnections)) {
                disconnect(connection);
            }
            m_metadataConnections.clear();
        }

        void clearBindings() {
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

        ListKind m_kind;
        QPointer<QObject> m_context;
        dspx::SelectionModel *m_selectionModel;
        SVS::MusicTimeline *m_musicTimeline;
        QList<ItemSelectorEntry> m_entries;
        QList<QMetaObject::Connection> m_ownerConnections;
        QList<QMetaObject::Connection> m_containerConnections;
        QList<QMetaObject::Connection> m_metadataConnections;
        QHash<QObject *, QList<QMetaObject::Connection>> m_objectConnections;
        QHash<QObject *, SingerNameResolver *> m_singerResolvers;
        QHash<QObject *, ParameterInfoProvider *> m_parameterProviders;
        SingerNameResolver *m_contextSingerResolver = nullptr;
        ParameterInfoProvider *m_anchorProvider = nullptr;
        KeySignatureAtSpecifiedPositionHelper *m_keySignatureHelper = nullptr;
    };

    class ItemSelectorDialogPrivate {
    public:
        struct Column {
            QWidget *widget;
            SelectAllCheckBox *selectAll;
            ItemSelectorView *view;
            ItemSelectorListModel *model;
        };

        ItemSelectorDialogPrivate(ItemSelectorDialog *dialog, ProjectWindowInterface *windowInterface)
            : q(dialog), windowInterface(windowInterface) {
        }

        void initialize() {
            q->setWindowTitle(ItemSelectorDialog::tr("Item Selector"));
            q->setWindowModality(Qt::WindowModal);
            q->resize(960, 620);

            auto *dialogLayout = new QVBoxLayout(q);
            scrollArea = new QScrollArea(q);
            scrollArea->setWidgetResizable(true);
            scrollArea->setFrameShape(QFrame::NoFrame);
            scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
            scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
            columnsWidget = new QWidget(scrollArea);
            columnsWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);
            columnsLayout = new QHBoxLayout(columnsWidget);
            columnsLayout->setContentsMargins(0, 0, 0, 0);
            columnsLayout->setSpacing(4);
            columnsLayout->setAlignment(Qt::AlignLeft);
            scrollArea->setWidget(columnsWidget);
            dialogLayout->addWidget(scrollArea, 1);

            auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close, q);
            QObject::connect(buttonBox, &QDialogButtonBox::rejected, q, &QDialog::close);
            dialogLayout->addWidget(buttonBox);

        }

        void prepareToShow() {
            if (columns.isEmpty()) {
                const bool updatesWereEnabled = q->updatesEnabled();
                if (updatesWereEnabled) {
                    q->setUpdatesEnabled(false);
                }
                addColumn(ListKind::Root, model());
                restoreInitialPath();
                if (updatesWereEnabled) {
                    q->setUpdatesEnabled(true);
                    q->update();
                }
                focusInitialColumn();
            }
        }

        void releaseColumns() {
            ++horizontalScrollRestoreGeneration;
            removeColumnsAfter(-1);
            scrollArea->horizontalScrollBar()->setValue(0);
        }

        dspx::Model *model() const {
            return windowInterface->projectDocumentContext()->document()->model();
        }

        dspx::SelectionModel *selectionModel() const {
            return windowInterface->projectDocumentContext()->document()->selectionModel();
        }

        void addColumn(ListKind kind, QObject *context) {
            auto *columnWidget = new QWidget(columnsWidget);
            columnWidget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
            auto *layout = new QVBoxLayout(columnWidget);
            layout->setContentsMargins(0, 0, 0, 0);
            layout->setSpacing(4);

            auto *model = new ItemSelectorListModel(kind, context, windowInterface, columnWidget);
            auto *selectAll = new SelectAllCheckBox(columnWidget);
            selectAll->setText(ItemSelectorDialog::tr("Select All"));
            selectAll->setAccessibleName(ItemSelectorDialog::tr("Select All Items in This Column"));
            selectAll->setVisible(model->isSelectableList());
            layout->addWidget(selectAll);

            auto *view = new ItemSelectorView(columnWidget);
            view->setAccessibleName(ItemSelectorDialog::tr("Item Hierarchy Column"));
            view->setModel(model);
            view->setHeaderHidden(true);
            view->header()->setStretchLastSection(false);
            view->header()->setSectionResizeMode(0, QHeaderView::Fixed);
            view->setRootIsDecorated(false);
            view->setItemsExpandable(false);
            view->setIndentation(0);
            view->setSelectionBehavior(QAbstractItemView::SelectRows);
            view->setSelectionMode(QAbstractItemView::SingleSelection);
            view->setEditTriggers(QAbstractItemView::NoEditTriggers);
            view->setAllColumnsShowFocus(true);
            view->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
            view->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
            view->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
            layout->addWidget(view, 1);

            columnsLayout->addWidget(columnWidget);
            columns.append({columnWidget, selectAll, view, model});
            updateColumnWidth(columnWidget);

            const auto updateSelectAll = [model, selectAll] {
                const QSignalBlocker blocker(selectAll);
                selectAll->setEnabled(model->hasSelectableItems());
                selectAll->setCheckState(model->checkStateSummary());
            };
            updateSelectAll();
            QObject::connect(model, &ItemSelectorListModel::checkStateSummaryChanged,
                             q, updateSelectAll);
            QObject::connect(model, &QAbstractItemModel::rowsInserted, q,
                             [this, columnWidget, updateSelectAll] {
                                 updateSelectAll();
                                 updateColumnWidth(columnWidget);
                             });
            QObject::connect(model, &QAbstractItemModel::rowsRemoved, q,
                             [this, columnWidget, updateSelectAll] {
                                 updateSelectAll();
                                 updateColumnWidth(columnWidget);
                             });
            QObject::connect(model, &QAbstractItemModel::modelReset, q,
                             [this, columnWidget] {
                                 updateColumnWidth(columnWidget);
                             });
            QObject::connect(model, &QAbstractItemModel::dataChanged, q,
                             [this, columnWidget](const QModelIndex &, const QModelIndex &,
                                                  const QList<int> &roles) {
                                 if (roles.isEmpty() || roles.contains(Qt::DisplayRole)) {
                                     updateColumnWidth(columnWidget);
                                 }
                             });
            QObject::connect(selectAll, &QCheckBox::clicked, q, [model, selectAll] {
                model->setAllSelected(selectAll->checkState() == Qt::Checked);
            });

            QObject::connect(view->selectionModel(), &QItemSelectionModel::currentChanged,
                             q, [this, columnWidget](const QModelIndex &current) {
                                 if (mutatingColumns) {
                                     return;
                                 }
                                 const int columnIndex = indexOfColumn(columnWidget);
                                 if (columnIndex >= 0) {
                                     navigateFrom(columnIndex, current);
                                 }
                             });
            QObject::connect(model, &QAbstractItemModel::rowsAboutToBeRemoved,
                             q, [this, columnWidget, view](const QModelIndex &, int first, int last) {
                                 const auto current = view->currentIndex();
                                 if (!current.isValid() || current.row() < first || current.row() > last) {
                                     return;
                                 }
                                 const bool wasMutatingColumns = mutatingColumns;
                                 mutatingColumns = true;
                                 view->setCurrentIndex({});
                                 mutatingColumns = wasMutatingColumns;
                                 const int columnIndex = indexOfColumn(columnWidget);
                                 if (columnIndex >= 0) {
                                     removeColumnsAfterPreservingScroll(columnIndex);
                                 }
                             });
            QObject::connect(model, &QAbstractItemModel::modelAboutToBeReset,
                             q, [this, columnWidget, view] {
                                 if (!view->currentIndex().isValid()) {
                                     return;
                                 }
                                 const bool wasMutatingColumns = mutatingColumns;
                                 mutatingColumns = true;
                                 view->setCurrentIndex({});
                                 mutatingColumns = wasMutatingColumns;
                                 const int columnIndex = indexOfColumn(columnWidget);
                                 if (columnIndex >= 0) {
                                     removeColumnsAfterPreservingScroll(columnIndex);
                                 }
                             });
            QObject::connect(view, &ItemSelectorView::focusPreviousRequested, q,
                             [this, columnWidget] {
                                 const int index = indexOfColumn(columnWidget);
                                 if (index > 0) {
                                     focusColumn(index - 1, true);
                                 }
                             });
            QObject::connect(view, &ItemSelectorView::focusNextRequested, q,
                             [this, columnWidget] {
                                 const int index = indexOfColumn(columnWidget);
                                 if (index >= 0 && index + 1 < columns.size()) {
                                     focusColumn(index + 1, true);
                                 }
                             });
        }

        void updateColumnWidth(QWidget *columnWidget) {
            const int columnIndex = indexOfColumn(columnWidget);
            if (columnIndex < 0) {
                return;
            }

            constexpr int minimumColumnWidth = 160;
            constexpr int maximumColumnWidth = 280;
            constexpr int maximumSampleCount = 120;

            const QPointer<QWidget> column = columns.at(columnIndex).widget;
            const QPointer<SelectAllCheckBox> selectAll = columns.at(columnIndex).selectAll;
            const QPointer<ItemSelectorView> view = columns.at(columnIndex).view;
            const QPointer<ItemSelectorListModel> model = columns.at(columnIndex).model;
            if (!column || !selectAll || !view || !model) {
                return;
            }

            int contentWidth = 0;
            const int rowCount = model->rowCount();
            const int sampleCount = std::min(rowCount, maximumSampleCount);
            for (int sample = 0; sample < sampleCount; ++sample) {
                const int row = sampleCount < 2
                                    ? 0
                                    : sample * (rowCount - 1) / (sampleCount - 1);
                contentWidth = std::max(
                    contentWidth,
                    view->sizeHintForIndex(model->index(row)).width());
            }

            const int contentPreferredWidth = contentWidth + view->frameWidth() * 2 + 12;
            const int selectAllPreferredWidth = selectAll->isVisible()
                                                    ? selectAll->sizeHint().width()
                                                    : 0;

            const int horizontalScrollValue = view->horizontalScrollBar()->value();
            const auto horizontalScrollBarPolicy = view->horizontalScrollBarPolicy();
            const bool updatesWereEnabled = view->updatesEnabled();
            if (updatesWereEnabled) {
                view->setUpdatesEnabled(false);
            }
            view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

            const auto applyColumnWidth = [this, column, view](int width) {
                column->setFixedWidth(width);
                columnsLayout->invalidate();
                columnsLayout->activate();
                if (column->layout()) {
                    column->layout()->invalidate();
                    column->layout()->activate();
                }
                view->doItemsLayout();
            };

            int targetWidth = std::clamp(
                std::max(contentPreferredWidth, selectAllPreferredWidth),
                minimumColumnWidth, maximumColumnWidth);
            applyColumnWidth(targetWidth);
            if (view->verticalScrollBar()->maximum()
                > view->verticalScrollBar()->minimum()) {
                const int scrollBarExtent = view->style()->pixelMetric(
                    QStyle::PM_ScrollBarExtent, nullptr, view);
                targetWidth = std::clamp(
                    std::max(contentPreferredWidth + scrollBarExtent,
                             selectAllPreferredWidth),
                    minimumColumnWidth, maximumColumnWidth);
                applyColumnWidth(targetWidth);
            }

            if (contentWidth <= view->viewport()->width()) {
                view->header()->setSectionResizeMode(0, QHeaderView::Stretch);
            } else {
                view->header()->setSectionResizeMode(0, QHeaderView::Fixed);
                view->setColumnWidth(0, contentWidth);
            }
            view->doItemsLayout();
            view->setHorizontalScrollBarPolicy(horizontalScrollBarPolicy);
            view->doItemsLayout();
            view->horizontalScrollBar()->setValue(
                std::min(horizontalScrollValue, view->horizontalScrollBar()->maximum()));
            if (updatesWereEnabled) {
                view->setUpdatesEnabled(true);
                view->update();
            }
        }

        void focusInitialColumn() {
            if (columns.isEmpty()) {
                return;
            }
            const QPointer<QWidget> column = columns.constLast().widget;
            const QPointer<ItemSelectorView> view = columns.constLast().view;
            QTimer::singleShot(0, q, [this, column, view] {
                if (!column || !view || !q->isVisible()) {
                    return;
                }
                const bool updatesWereEnabled = q->updatesEnabled();
                if (updatesWereEnabled) {
                    q->setUpdatesEnabled(false);
                }
                const auto columnSnapshot = columns;
                for (const auto &currentColumn : columnSnapshot) {
                    if (currentColumn.widget) {
                        updateColumnWidth(currentColumn.widget);
                    }
                }
                if (updatesWereEnabled) {
                    q->setUpdatesEnabled(true);
                    q->update();
                }
                view->setFocus();
                scrollArea->ensureWidgetVisible(column, columnsLayout->spacing(), 0);
            });
        }

        void focusColumn(int columnIndex, bool ensureVisible) {
            if (columnIndex < 0 || columnIndex >= columns.size()) {
                return;
            }
            const QPointer<QWidget> column = columns.at(columnIndex).widget;
            const QPointer<ItemSelectorView> view = columns.at(columnIndex).view;
            if (!column || !view) {
                return;
            }
            view->setFocus();
            if (ensureVisible) {
                scrollArea->ensureWidgetVisible(column, columnsLayout->spacing(), 0);
            }
        }

        void restoreHorizontalScrollPosition(int value) {
            const int generation = ++horizontalScrollRestoreGeneration;
            const QPointer<QScrollBar> scrollBar = scrollArea->horizontalScrollBar();
            if (!scrollBar) {
                return;
            }
            scrollBar->setValue(std::clamp(value, scrollBar->minimum(), scrollBar->maximum()));
            QTimer::singleShot(0, q, [this, generation, scrollBar, value] {
                if (generation != horizontalScrollRestoreGeneration || !scrollBar) {
                    return;
                }
                scrollBar->setValue(std::clamp(value,
                                               scrollBar->minimum(), scrollBar->maximum()));
            });
        }

        void removeColumnsAfterPreservingScroll(int columnIndex) {
            const int horizontalScrollValue = scrollArea->horizontalScrollBar()->value();
            const bool updatesWereEnabled = q->updatesEnabled();
            if (updatesWereEnabled) {
                q->setUpdatesEnabled(false);
            }
            removeColumnsAfter(columnIndex);
            columnsWidget->updateGeometry();
            columnsLayout->invalidate();
            columnsLayout->activate();
            if (updatesWereEnabled) {
                q->setUpdatesEnabled(true);
                q->update();
            }
            restoreHorizontalScrollPosition(horizontalScrollValue);
        }

        void navigateFrom(int columnIndex, const QModelIndex &current) {
            if (columnIndex < 0 || columnIndex >= columns.size()) {
                return;
            }

            bool hasNextColumn = false;
            ListKind nextKind = ListKind::Root;
            QObject *nextContext = nullptr;

            const QPointer<ItemSelectorListModel> columnModel = columns.at(columnIndex).model;
            if (!columnModel) {
                return;
            }
            const auto *sourceEntry = current.isValid() ? columnModel->entryAt(current.row()) : nullptr;
            const ItemSelectorEntry entry = sourceEntry ? *sourceEntry : ItemSelectorEntry{};

            if (sourceEntry) {
                switch (entry.kind) {
                case NodeKind::RootTracks:
                    hasNextColumn = true;
                    nextKind = ListKind::Tracks;
                    nextContext = model();
                    break;
                case NodeKind::RootLabels:
                    hasNextColumn = true;
                    nextKind = ListKind::Labels;
                    nextContext = model();
                    break;
                case NodeKind::RootTempos:
                    hasNextColumn = true;
                    nextKind = ListKind::Tempos;
                    nextContext = model();
                    break;
                case NodeKind::RootKeySignatures:
                    hasNextColumn = true;
                    nextKind = ListKind::KeySignatures;
                    nextContext = model();
                    break;
                case NodeKind::Track:
                    hasNextColumn = true;
                    nextKind = ListKind::Clips;
                    nextContext = entry.object.data();
                    break;
                case NodeKind::Clip: {
                    auto *clip = qobject_cast<dspx::Clip *>(entry.object.data());
                    if (clip && clip->type() == dspx::Clip::Singing) {
                        hasNextColumn = true;
                        nextKind = ListKind::SingingBranches;
                        nextContext = clip;
                    }
                    break;
                }
                case NodeKind::SingingNotes:
                    hasNextColumn = true;
                    nextKind = ListKind::Notes;
                    nextContext = columnModel->context();
                    break;
                case NodeKind::SingingParameters:
                    hasNextColumn = true;
                    nextKind = ListKind::Parameters;
                    nextContext = columnModel->context();
                    break;
                case NodeKind::SingingVoiceBlending:
                    hasNextColumn = true;
                    nextKind = ListKind::DynamicAnchors;
                    nextContext = columnModel->context();
                    break;
                case NodeKind::Parameter:
                    hasNextColumn = true;
                    nextKind = ListKind::AnchorBranches;
                    nextContext = entry.object.data();
                    break;
                case NodeKind::EditedAnchors: {
                    auto *parameter = qobject_cast<dspx::Parameter *>(columnModel->context());
                    hasNextColumn = true;
                    nextKind = ListKind::Anchors;
                    nextContext = parameter ? parameter->anchorEdited() : nullptr;
                    break;
                }
                case NodeKind::TransformAnchors: {
                    auto *parameter = qobject_cast<dspx::Parameter *>(columnModel->context());
                    hasNextColumn = true;
                    nextKind = ListKind::Anchors;
                    nextContext = parameter ? parameter->anchorTransform() : nullptr;
                    break;
                }
                default:
                    break;
                }
            }

            const int nextColumnIndex = columnIndex + 1;
            if (hasNextColumn && nextColumnIndex < columns.size()) {
                const QPointer<ItemSelectorListModel> nextColumnModel = columns.at(nextColumnIndex).model;
                if (nextColumnModel && nextColumnModel->listKind() == nextKind
                    && nextColumnModel->context() == nextContext) {
                    return;
                }
            } else if (!hasNextColumn && nextColumnIndex == columns.size()) {
                return;
            }

            const int horizontalScrollValue = scrollArea->horizontalScrollBar()->value();
            const bool updatesWereEnabled = q->updatesEnabled();
            if (updatesWereEnabled) {
                q->setUpdatesEnabled(false);
            }
            removeColumnsAfter(columnIndex);
            if (hasNextColumn) {
                addColumn(nextKind, nextContext);
            }
            columnsWidget->updateGeometry();
            columnsLayout->invalidate();
            columnsLayout->activate();
            if (updatesWereEnabled) {
                q->setUpdatesEnabled(true);
                q->update();
            }
            restoreHorizontalScrollPosition(horizontalScrollValue);
        }

        void removeColumnsAfter(int columnIndex) {
            if (columnIndex < -1) {
                return;
            }
            const bool wasMutatingColumns = mutatingColumns;
            mutatingColumns = true;
            while (columns.size() > columnIndex + 1) {
                auto column = columns.takeLast();
                columnsLayout->removeWidget(column.widget);
                delete column.widget;
            }
            mutatingColumns = wasMutatingColumns;
        }

        int indexOfColumn(QWidget *widget) const {
            for (int index = 0; index < columns.size(); ++index) {
                if (columns.at(index).widget == widget) {
                    return index;
                }
            }
            return -1;
        }

        bool activate(int columnIndex, NodeKind kind, QObject *object = nullptr,
                      const QString &key = {}) {
            if (columnIndex < 0 || columnIndex >= columns.size()) {
                return false;
            }
            const QPointer<ItemSelectorListModel> columnModel = columns.at(columnIndex).model;
            const QPointer<ItemSelectorView> columnView = columns.at(columnIndex).view;
            if (!columnModel || !columnView) {
                return false;
            }
            const int row = columnModel->rowFor(kind, object, key);
            if (row < 0) {
                return false;
            }
            const int horizontalScrollValue = columnView->horizontalScrollBar()->value();
            const auto target = columnModel->index(row);
            columnView->setCurrentIndex(target);
            if (!columnView || !columnModel) {
                return false;
            }
            columnView->scrollTo(target, QAbstractItemView::PositionAtCenter);
            columnView->horizontalScrollBar()->setValue(
                std::min(horizontalScrollValue,
                         columnView->horizontalScrollBar()->maximum()));
            return true;
        }

        void restoreInitialPath() {
            auto *selection = selectionModel();
            switch (selection->selectionType()) {
                case dspx::SelectionModel::ST_None:
                    return;
                case dspx::SelectionModel::ST_Track:
                    activate(0, NodeKind::RootTracks);
                    return;
                case dspx::SelectionModel::ST_Label:
                    activate(0, NodeKind::RootLabels);
                    return;
                case dspx::SelectionModel::ST_Tempo:
                    activate(0, NodeKind::RootTempos);
                    return;
                case dspx::SelectionModel::ST_KeySignature:
                    activate(0, NodeKind::RootKeySignatures);
                    return;
                case dspx::SelectionModel::ST_Clip:
                    restoreClipPath();
                    return;
                case dspx::SelectionModel::ST_Note:
                    restoreNotePath();
                    return;
                case dspx::SelectionModel::ST_AnchorNode:
                    restoreAnchorPath();
                    return;
                case dspx::SelectionModel::ST_DynamicMixingAnchor:
                    restoreDynamicAnchorPath();
                    return;
            }
        }

        void restoreClipPath() {
            auto *clipSelection = selectionModel()->clipSelectionModel();
            dspx::Track *targetTrack = nullptr;
            int targetTrackIndex = std::numeric_limits<int>::max();
            const auto tracks = model()->tracks()->items();
            for (auto *clip : clipSelection->selectedItems()) {
                auto *track = clip && clip->clipSequence() ? clip->clipSequence()->track() : nullptr;
                const int index = tracks.indexOf(track);
                if (index >= 0 && index < targetTrackIndex) {
                    targetTrack = track;
                    targetTrackIndex = index;
                }
            }
            if (!targetTrack) {
                auto *clip = clipSelection->currentItem();
                targetTrack = clip && clip->clipSequence() ? clip->clipSequence()->track() : nullptr;
            }
            if (!targetTrack || !activate(0, NodeKind::RootTracks)) {
                return;
            }
            activate(1, NodeKind::Track, targetTrack);
        }

        void restoreNotePath() {
            auto *sequence = selectionModel()->noteSelectionModel()->noteSequenceWithSelectedItems();
            auto *clip = sequence ? sequence->singingClip() : nullptr;
            auto *track = clip && clip->clipSequence() ? clip->clipSequence()->track() : nullptr;
            if (!track || !activate(0, NodeKind::RootTracks)
                || !activate(1, NodeKind::Track, track)
                || !activate(2, NodeKind::Clip, clip)) {
                return;
            }
            activate(3, NodeKind::SingingNotes);
        }

        void restoreAnchorPath() {
            auto *sequence = selectionModel()->anchorNodeSelectionModel()->anchorNodeSequenceWithSelectedItems();
            auto *parameter = sequence ? sequence->parameter() : nullptr;
            auto *parameterMap = parameter ? parameter->parameterMap() : nullptr;
            auto *clip = parameterMap ? parameterMap->singingClip() : nullptr;
            auto *track = clip && clip->clipSequence() ? clip->clipSequence()->track() : nullptr;
            if (!track || !parameter || !activate(0, NodeKind::RootTracks)
                || !activate(1, NodeKind::Track, track)
                || !activate(2, NodeKind::Clip, clip)
                || !activate(3, NodeKind::SingingParameters)
                || !activate(4, NodeKind::Parameter, parameter)) {
                return;
            }
            activate(5, sequence->role() == dspx::AnchorNodeSequence::Edited
                            ? NodeKind::EditedAnchors
                            : NodeKind::TransformAnchors);
        }

        void restoreDynamicAnchorPath() {
            auto *sequence = selectionModel()->dynamicMixingAnchorSelectionModel()
                                 ->dynamicMixingAnchorSequenceWithSelectedItems();
            auto *sources = sequence ? sequence->sources() : nullptr;
            auto *clip = sources ? sources->singingClip() : nullptr;
            auto *track = clip && clip->clipSequence() ? clip->clipSequence()->track() : nullptr;
            if (!track || !activate(0, NodeKind::RootTracks)
                || !activate(1, NodeKind::Track, track)
                || !activate(2, NodeKind::Clip, clip)) {
                return;
            }
            activate(3, NodeKind::SingingVoiceBlending);
        }

        ItemSelectorDialog *q;
        ProjectWindowInterface *windowInterface;
        QScrollArea *scrollArea = nullptr;
        QWidget *columnsWidget = nullptr;
        QHBoxLayout *columnsLayout = nullptr;
        QList<Column> columns;
        bool mutatingColumns = false;
        int horizontalScrollRestoreGeneration = 0;
    };

    ItemSelectorDialog::ItemSelectorDialog(ProjectWindowInterface *windowInterface, QWidget *parent)
        : QDialog(parent), d(new ItemSelectorDialogPrivate(this, windowInterface)) {
        d->initialize();
    }

    ItemSelectorDialog::~ItemSelectorDialog() {
        d->releaseColumns();
        delete d;
    }

    void ItemSelectorDialog::hideEvent(QHideEvent *event) {
        d->releaseColumns();
        QDialog::hideEvent(event);
    }

    void ItemSelectorDialog::showEvent(QShowEvent *event) {
        QDialog::showEvent(event);
        d->prepareToShow();
    }

}

#include "ItemSelectorDialog.moc"
