// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "EffectsContext.h"

#include <algorithm>
#include <iterator>
#include <utility>

#include <QHash>
#include <QLoggingCategory>
#include <QMutex>
#include <QMutexLocker>
#include <QQuickItem>

#include <TalcsCore/AudioSource.h>
#include <TalcsCore/PositionableMixerAudioSource.h>

#include <dspxmodelORM/AudioDSP.h>
#include <dspxmodelORM/AudioDSPList.h>
#include <dspxmodelORM/Model.h>

#include <coreplugin/DspxDocument.h>
#include <coreplugin/ProjectDocumentContext.h>
#include <coreplugin/ProjectWindowInterface.h>

#include <effectsunitmanager/EffectsUnit.h>
#include <effectsunitmanager/EffectsUnitClass.h>
#include <effectsunitmanager/EffectsUnitCollection.h>

#include <transactional/TransactionController.h>

namespace EffectsUnitManager::Internal {

    Q_STATIC_LOGGING_CATEGORY(lcEffectsContext, "diffscope.effectsunitmanager.effectscontext")

    class EffectsChainFilter : public talcs::AudioSource {
    public:
        struct Slot {
            talcs::AudioSource *processor{};
            bool enabled{};
            bool opened{};
        };

        bool open(qint64 bufferSize, double sampleRate) override {
            if (!AudioSource::open(bufferSize, sampleRate)) {
                return false;
            }
            QMutexLocker locker(&m_mutex);
            for (auto &slot : m_slots) {
                slot.opened = slot.processor->open(bufferSize, sampleRate);
                if (!slot.opened) {
                    qCWarning(lcEffectsContext) << "Failed to open effect processor" << slot.processor;
                }
            }
            return true;
        }

        void close() override {
            QMutexLocker locker(&m_mutex);
            for (auto &slot : m_slots) {
                if (slot.opened) {
                    slot.processor->close();
                    slot.opened = false;
                }
            }
            AudioSource::close();
        }

        void setProcessors(const QList<QPair<talcs::AudioSource *, bool>> &processors) {
            QMutexLocker locker(&m_mutex);
            QList<Slot> newSlots;
            newSlots.reserve(processors.size());
            for (const auto &[processor, enabled] : processors) {
                auto old = std::ranges::find_if(m_slots, [processor](const Slot &slot) {
                    return slot.processor == processor;
                });
                bool opened = old != m_slots.end() && old->opened;
                if (isOpen() && old == m_slots.end()) {
                    opened = processor->open(bufferSize(), sampleRate());
                    if (!opened) {
                        qCWarning(lcEffectsContext) << "Failed to open effect processor" << processor;
                    }
                }
                newSlots.append({processor, enabled, opened});
            }
            for (auto &oldSlot : m_slots) {
                const bool retained = std::ranges::any_of(newSlots, [&oldSlot](const Slot &slot) {
                    return slot.processor == oldSlot.processor;
                });
                if (!retained && oldSlot.opened) {
                    oldSlot.processor->close();
                }
            }
            m_slots = std::move(newSlots);
        }

    protected:
        qint64 processReading(const talcs::AudioSourceReadData &readData) override {
            QMutexLocker locker(&m_mutex);
            for (const auto &slot : std::as_const(m_slots)) {
                if (slot.enabled && slot.opened) {
                    slot.processor->read(readData);
                }
            }
            return readData.length;
        }

    private:
        QMutex m_mutex;
        QList<Slot> m_slots;
    };

    struct EffectsContext::Entry {
        dspx::AudioDSP *item{};
        EffectsUnit *unit{};
        bool expanded{true};
        bool applyingState{};
    };

    EffectsContext::EffectsContext(Core::ProjectWindowInterface *windowHandle,
                                   dspx::AudioDSPList *audioDSPList,
                                   talcs::PositionableMixerAudioSource *mixer,
                                   QObject *parent)
        : QAbstractListModel(parent),
          m_windowHandle(windowHandle),
          m_audioDSPList(audioDSPList),
          m_mixer(mixer),
          m_filter(std::make_unique<EffectsChainFilter>()) {
        Q_ASSERT(m_windowHandle);
        Q_ASSERT(m_audioDSPList);
        Q_ASSERT(m_mixer);

        if (m_mixer->readingFilter()) {
            m_readingFilterConflict = true;
            qCWarning(lcEffectsContext) << "Effects mixer already has a reading filter" << m_mixer;
        } else {
            m_mixer->setReadingFilter(m_filter.get());
        }

        for (int index = 0; index < m_audioDSPList->size(); ++index) {
            insertEntry(index, m_audioDSPList->item(index));
        }
        updateAudioChain();

        connect(m_audioDSPList, &dspx::AudioDSPList::itemAboutToInsert, this,
                [this](int index, dspx::AudioDSP *) {
                    beginInsertRows({}, index, index);
                });
        connect(m_audioDSPList, &dspx::AudioDSPList::itemInserted, this,
                [this](int index, dspx::AudioDSP *item) {
                    insertEntry(index, item);
                    endInsertRows();
                    updateAudioChain();
                });
        connect(m_audioDSPList, &dspx::AudioDSPList::itemAboutToRemove, this,
                [this](int index, dspx::AudioDSP *) {
                    beginRemoveRows({}, index, index);
                });
        connect(m_audioDSPList, &dspx::AudioDSPList::itemRemoved, this,
                [this](int index, dspx::AudioDSP *) {
                    removeEntry(index);
                    endRemoveRows();
                });
        connect(m_audioDSPList, &dspx::AudioDSPList::aboutToRotate, this,
                [this](int, int, int) {
                    beginResetModel();
                });
        connect(m_audioDSPList, &dspx::AudioDSPList::rotated, this,
                [this](int left, int middle, int right) {
                    std::rotate(m_entries.begin() + left, m_entries.begin() + middle, m_entries.begin() + right);
                    endResetModel();
                    updateAudioChain();
                });
        connect(EffectsUnitCollection::instance(), &EffectsUnitCollection::effectsUnitClassRegistered,
                this, [this](const QString &id, EffectsUnitClass *) {
                    for (int row = 0; row < static_cast<int>(m_entries.size()); ++row) {
                        auto &entry = *m_entries.at(row);
                        if (!entry.unit && entry.item->id() == id) {
                            createUnit(entry);
                            const auto modelIndex = index(row);
                            Q_EMIT dataChanged(modelIndex, modelIndex,
                                               {NameRole, KnownRole, EditorRole, ErrorRole});
                        }
                    }
                    updateAudioChain();
                });
    }

    EffectsContext::~EffectsContext() {
        m_filter->setProcessors({});
    }

    bool EffectsContext::readingFilterConflict() const {
        return m_readingFilterConflict;
    }

    int EffectsContext::rowCount(const QModelIndex &parent) const {
        return parent.isValid() ? 0 : static_cast<int>(m_entries.size());
    }

    QVariant EffectsContext::data(const QModelIndex &index, int role) const {
        const auto entry = entryAt(index.row());
        if (!entry || index.column() != 0) {
            return {};
        }
        switch (role) {
            case IdRole:
                return entry->item->id();
            case NameRole: {
                auto effectsUnitClass = EffectsUnitCollection::instance()->effectsUnitClass(entry->item->id());
                return effectsUnitClass ? effectsUnitClass->name() : tr("Unknown Effect");
            }
            case EnabledRole:
                return entry->item->enabled();
            case KnownRole:
                return entry->unit != nullptr;
            case EditorRole:
                return QVariant::fromValue(entry->unit ? entry->unit->editor() : nullptr);
            case ExpandedRole:
                return entry->expanded;
            case ErrorRole:
                return entry->unit ? QString() : tr("The effect \"%1\" is not registered.").arg(entry->item->id());
            default:
                return {};
        }
    }

    QHash<int, QByteArray> EffectsContext::roleNames() const {
        return {
            {IdRole, "effectId"},
            {NameRole, "name"},
            {EnabledRole, "enabled"},
            {KnownRole, "known"},
            {EditorRole, "editor"},
            {ExpandedRole, "expanded"},
            {ErrorRole, "error"},
        };
    }

    bool EffectsContext::addEffect(const QString &id) {
        auto effectsUnitClass = EffectsUnitCollection::instance()->effectsUnitClass(id);
        if (!effectsUnitClass) {
            return false;
        }
        auto document = m_windowHandle->projectDocumentContext()->document();
        bool inserted = false;
        const bool transactionStarted = document->transactionController()->beginScopedTransaction(
            tr("Adding effect"), [this, document, effectsUnitClass, &inserted, &id] {
                auto unit = effectsUnitClass->create(this);
                Q_ASSERT(unit);
                if (!unit) {
                    return false;
                }
                auto item = document->model()->createAudioDSP();
                item->setId(id);
                item->setData(unit->getState());
                item->setEnabled(true);
                m_pendingItem = item;
                m_pendingUnit = unit;
                if (!m_audioDSPList->insertItem(m_audioDSPList->size(), item)) {
                    m_pendingItem = nullptr;
                    m_pendingUnit = nullptr;
                    delete unit;
                    document->model()->destroyItem(item);
                    return false;
                }
                Q_ASSERT(!m_pendingUnit);
                m_pendingItem = nullptr;
                m_pendingUnit = nullptr;
                inserted = true;
                return true;
            });
        return transactionStarted && inserted;
    }

    bool EffectsContext::removeEffect(int row) {
        if (!entryAt(row)) {
            return false;
        }
        auto document = m_windowHandle->projectDocumentContext()->document();
        bool removed = false;
        const bool transactionStarted = document->transactionController()->beginScopedTransaction(
            tr("Removing effect"), [this, document, row, &removed] {
                auto item = m_audioDSPList->removeItem(row);
                if (!item) {
                    return false;
                }
                document->model()->destroyItem(item);
                removed = true;
                return true;
            });
        return transactionStarted && removed;
    }

    bool EffectsContext::setEffectEnabled(int row, bool enabled) {
        auto entry = entryAt(row);
        if (!entry || entry->item->enabled() == enabled) {
            return false;
        }
        auto document = m_windowHandle->projectDocumentContext()->document();
        bool changed = false;
        const bool transactionStarted = document->transactionController()->beginScopedTransaction(
            enabled ? tr("Enabling effect") : tr("Disabling effect"),
            [entry, enabled, &changed] {
                entry->item->setEnabled(enabled);
                changed = true;
                return true;
            });
        return transactionStarted && changed;
    }

    bool EffectsContext::moveEffect(int row, int offset) {
        if (!entryAt(row) || (offset != -1 && offset != 1)) {
            return false;
        }
        const int target = row + offset;
        if (target < 0 || target >= rowCount()) {
            return false;
        }
        auto document = m_windowHandle->projectDocumentContext()->document();
        bool moved = false;
        const bool transactionStarted = document->transactionController()->beginScopedTransaction(
            tr("Moving effect"), [this, row, offset, &moved] {
                moved = offset < 0
                    ? m_audioDSPList->rotate(row - 1, row, row + 1)
                    : m_audioDSPList->rotate(row, row + 1, row + 2);
                return moved;
            });
        return transactionStarted && moved;
    }

    void EffectsContext::setExpanded(int row, bool expanded) {
        auto entry = entryAt(row);
        if (!entry || entry->expanded == expanded) {
            return;
        }
        entry->expanded = expanded;
        const auto modelIndex = index(row);
        Q_EMIT dataChanged(modelIndex, modelIndex, {ExpandedRole});
    }

    EffectsContext::Entry *EffectsContext::entryAt(int row) const {
        if (row < 0 || row >= static_cast<int>(m_entries.size())) {
            return nullptr;
        }
        return m_entries.at(row).get();
    }

    int EffectsContext::indexOf(dspx::AudioDSP *item) const {
        const auto it = std::ranges::find_if(m_entries, [item](const auto &entry) {
            return entry->item == item;
        });
        return it == m_entries.end() ? -1 : static_cast<int>(std::distance(m_entries.begin(), it));
    }

    void EffectsContext::insertEntry(int index, dspx::AudioDSP *item) {
        auto entry = std::make_unique<Entry>();
        entry->item = item;
        auto existingUnit = item == m_pendingItem ? std::exchange(m_pendingUnit, nullptr) : nullptr;
        createUnit(*entry, existingUnit);
        m_entries.insert(m_entries.begin() + index, std::move(entry));

        connect(item, &dspx::AudioDSP::idChanged, this, [this, item] {
            const int row = indexOf(item);
            if (auto entry = entryAt(row)) {
                recreateUnit(*entry);
                const auto modelIndex = this->index(row);
                Q_EMIT dataChanged(modelIndex, modelIndex,
                                   {IdRole, NameRole, KnownRole, EditorRole, ErrorRole});
            }
        });
        connect(item, &dspx::AudioDSP::dataChanged, this, [this, item](const QJsonValue &) {
            if (auto entry = entryAt(indexOf(item)); entry && entry->unit) {
                restoreUnitState(*entry);
            }
        });
        connect(item, &dspx::AudioDSP::enabledChanged, this, [this, item](bool) {
            const int row = indexOf(item);
            if (row < 0) {
                return;
            }
            const auto modelIndex = this->index(row);
            Q_EMIT dataChanged(modelIndex, modelIndex, {EnabledRole});
            updateAudioChain();
        });
    }

    void EffectsContext::removeEntry(int index) {
        if (index < 0 || index >= static_cast<int>(m_entries.size())) {
            return;
        }
        auto entry = std::move(m_entries.at(index));
        m_entries.erase(m_entries.begin() + index);
        disconnect(entry->item, nullptr, this, nullptr);
        auto unit = entry->unit;
        entry->unit = nullptr;
        updateAudioChain();
        delete unit;
    }

    void EffectsContext::recreateUnit(Entry &entry) {
        auto oldUnit = entry.unit;
        entry.unit = nullptr;
        updateAudioChain();
        delete oldUnit;
        createUnit(entry);
        updateAudioChain();
    }

    void EffectsContext::createUnit(Entry &entry, EffectsUnit *existingUnit) {
        auto effectsUnitClass = EffectsUnitCollection::instance()->effectsUnitClass(entry.item->id());
        if (!effectsUnitClass) {
            Q_ASSERT(!existingUnit);
            return;
        }
        entry.unit = existingUnit ? existingUnit : effectsUnitClass->create(this);
        Q_ASSERT(entry.unit);
        if (!entry.unit) {
            return;
        }
        Q_ASSERT(entry.unit->editor());
        Q_ASSERT(entry.unit->processor());
        if (entry.unit->parent() != this) {
            entry.unit->setParent(this);
        }
        connect(entry.unit, &EffectsUnit::updated, this, [this, item = entry.item] {
            handleUnitUpdated(item);
        });
        restoreUnitState(entry);
    }

    void EffectsContext::updateAudioChain() {
        QList<QPair<talcs::AudioSource *, bool>> processors;
        processors.reserve(static_cast<qsizetype>(m_entries.size()));
        for (const auto &entry : m_entries) {
            if (entry->unit) {
                processors.append({entry->unit->processor(), entry->item->enabled()});
            }
        }
        m_filter->setProcessors(processors);
    }

    void EffectsContext::handleUnitUpdated(dspx::AudioDSP *item) {
        auto entry = entryAt(indexOf(item));
        if (!entry || !entry->unit || entry->applyingState) {
            return;
        }
        const auto state = entry->unit->getState();
        if (state == item->data()) {
            return;
        }
        auto document = m_windowHandle->projectDocumentContext()->document();
        bool changed = false;
        const bool transactionStarted = document->transactionController()->beginScopedTransaction(
            tr("Editing effect"), [this, item, state, &changed] {
                auto currentEntry = entryAt(indexOf(item));
                if (!currentEntry || currentEntry->unit->getState() != state) {
                    return false;
                }
                item->setData(state);
                changed = true;
                return true;
            });
        if (!transactionStarted || !changed) {
            if (auto currentEntry = entryAt(indexOf(item))) {
                restoreUnitState(*currentEntry);
            }
        }
    }

    void EffectsContext::restoreUnitState(Entry &entry) {
        if (!entry.unit) {
            return;
        }
        entry.applyingState = true;
        entry.unit->setState(entry.item->data());
        entry.applyingState = false;
    }

}

#include "moc_EffectsContext.cpp"
