#include "PhonemeListModel.h"

#include <dspxmodel/Phoneme.h>
#include <dspxmodel/PhonemeSequence.h>

namespace Core::Internal {

    PhonemeListModel::PhonemeListModel(QObject *parent) : QAbstractItemModel(parent) {
    }

    PhonemeListModel::~PhonemeListModel() {
        resetModel();
    }

    dspx::PhonemeSequence *PhonemeListModel::phonemeSequence() const {
        return m_phonemeSequence;
    }

    void PhonemeListModel::setPhonemeSequence(dspx::PhonemeSequence *phonemeSequence) {
        if (m_phonemeSequence == phonemeSequence)
            return;

        resetModel();

        m_phonemeSequence = phonemeSequence;

        if (m_phonemeSequence) {
            connect(m_phonemeSequence, &dspx::PhonemeSequence::itemInserted, this, &PhonemeListModel::onItemInserted);
            connect(m_phonemeSequence, &dspx::PhonemeSequence::itemRemoved, this, &PhonemeListModel::onItemRemoved);
            populateFromSequence();
        }

        Q_EMIT phonemeSequenceChanged();
    }

    QModelIndex PhonemeListModel::index(int row, int column, const QModelIndex &parent) const {
        if (parent.isValid() || row < 0 || row >= m_phonemes.size() || column != 0)
            return {};

        return createIndex(row, column, m_phonemes[row]);
    }

    QModelIndex PhonemeListModel::parent(const QModelIndex &child) const {
        Q_UNUSED(child)
        return {};
    }

    int PhonemeListModel::rowCount(const QModelIndex &parent) const {
        if (parent.isValid())
            return 0;

        return m_phonemes.size();
    }

    int PhonemeListModel::columnCount(const QModelIndex &parent) const {
        Q_UNUSED(parent)
        return 1;
    }

    QVariant PhonemeListModel::data(const QModelIndex &index, int role) const {
        if (!index.isValid() || index.row() >= m_phonemes.size())
            return {};

        auto *phoneme = m_phonemes[index.row()];
        if (!phoneme)
            return {};

        switch (role) {
            case TokenRole:
                return phoneme->token();
            case LanguageRole:
                return phoneme->language();
            case StartRole:
                return phoneme->start();
            case OnsetRole:
                return phoneme->onset();
            default:
                return {};
        }
    }

    bool PhonemeListModel::setData(const QModelIndex &index, const QVariant &value, int role) {
        if (!index.isValid() || index.row() >= m_phonemes.size())
            return false;

        auto *phoneme = m_phonemes[index.row()];
        if (!phoneme)
            return false;

        switch (role) {
            case TokenRole:
                phoneme->setToken(value.toString());
                return true;
            case LanguageRole:
                phoneme->setLanguage(value.toString());
                return true;
            case StartRole:
                phoneme->setStart(value.toInt());
                return true;
            case OnsetRole:
                phoneme->setOnset(value.toBool());
                return true;
            default:
                return false;
        }
    }

    Qt::ItemFlags PhonemeListModel::flags(const QModelIndex &index) const {
        if (!index.isValid())
            return Qt::NoItemFlags;

        return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
    }

    QHash<int, QByteArray> PhonemeListModel::roleNames() const {
        static const QHash<int, QByteArray> roles{
            {TokenRole, "token"},
            {LanguageRole, "language"},
            {StartRole, "start"},
            {OnsetRole, "onset"}
        };
        return roles;
    }

    void PhonemeListModel::resetModel() {
        beginResetModel();

        for (auto *phoneme : m_phonemes) {
            detachPhonemeSignals(phoneme);
        }
        m_phonemes.clear();

        if (m_phonemeSequence) {
            disconnect(m_phonemeSequence, nullptr, this, nullptr);
            m_phonemeSequence = nullptr;
        }

        endResetModel();
    }

    void PhonemeListModel::populateFromSequence() {
        if (!m_phonemeSequence)
            return;

        beginResetModel();

        // Collect all phonemes from sequence
        for (auto *phoneme : m_phonemeSequence->asRange()) {
            if (phoneme) {
                m_phonemes.append(phoneme);
            }
        }

        // Sort by start position
        std::sort(m_phonemes.begin(), m_phonemes.end(), [](dspx::Phoneme *a, dspx::Phoneme *b) {
            return a->start() < b->start();
        });

        // Attach signals
        for (auto *phoneme : m_phonemes) {
            attachPhonemeSignals(phoneme);
        }

        endResetModel();
    }

    void PhonemeListModel::attachPhonemeSignals(dspx::Phoneme *phoneme) {
        if (!phoneme)
            return;

        connect(phoneme, &dspx::Phoneme::startChanged, this, [this, phoneme](int start) {
            onPhonemeStartChanged(phoneme, start);
        });

        connect(phoneme, &dspx::Phoneme::tokenChanged, this, [this, phoneme]() {
            onPhonemeDataChanged(phoneme);
        });

        connect(phoneme, &dspx::Phoneme::languageChanged, this, [this, phoneme]() {
            onPhonemeDataChanged(phoneme);
        });

        connect(phoneme, &dspx::Phoneme::onsetChanged, this, [this, phoneme]() {
            onPhonemeDataChanged(phoneme);
        });
    }

    void PhonemeListModel::detachPhonemeSignals(dspx::Phoneme *phoneme) {
        if (!phoneme)
            return;

        disconnect(phoneme, nullptr, this, nullptr);
    }

    int PhonemeListModel::findInsertPosition(int start) const {
        // Binary search for insertion position
        int left = 0;
        int right = m_phonemes.size();

        while (left < right) {
            int mid = left + (right - left) / 2;
            if (m_phonemes[mid]->start() < start) {
                left = mid + 1;
            } else {
                right = mid;
            }
        }

        return left;
    }

    int PhonemeListModel::findPhonemeRow(dspx::Phoneme *phoneme) const {
        for (int i = 0; i < m_phonemes.size(); ++i) {
            if (m_phonemes[i] == phoneme)
                return i;
        }
        return -1;
    }

    void PhonemeListModel::reorderPhoneme(dspx::Phoneme *phoneme) {
        int oldRow = findPhonemeRow(phoneme);
        if (oldRow == -1)
            return;

        int newRow = findInsertPosition(phoneme->start());
        
        // Adjust newRow if the phoneme is moving forward
        if (newRow > oldRow) {
            newRow--;
        }

        // If position doesn't change, just notify data changed
        if (oldRow == newRow) {
            auto idx = index(oldRow, 0);
            Q_EMIT dataChanged(idx, idx, {StartRole});
            return;
        }

        // Move the phoneme to the new position
        if (!beginMoveRows(QModelIndex(), oldRow, oldRow, QModelIndex(), newRow > oldRow ? newRow + 1 : newRow)) {
            return;
        }

        m_phonemes.move(oldRow, newRow);
        endMoveRows();
    }

    void PhonemeListModel::onItemInserted(dspx::Phoneme *phoneme) {
        if (!phoneme)
            return;

        // Find the correct position to insert
        int row = findInsertPosition(phoneme->start());

        beginInsertRows(QModelIndex(), row, row);
        m_phonemes.insert(row, phoneme);
        endInsertRows();

        attachPhonemeSignals(phoneme);
    }

    void PhonemeListModel::onItemRemoved(dspx::Phoneme *phoneme) {
        if (!phoneme)
            return;

        int row = findPhonemeRow(phoneme);
        if (row == -1)
            return;

        detachPhonemeSignals(phoneme);

        beginRemoveRows(QModelIndex(), row, row);
        m_phonemes.removeAt(row);
        endRemoveRows();
    }

    void PhonemeListModel::onPhonemeStartChanged(dspx::Phoneme *phoneme, int start) {
        Q_UNUSED(start)
        reorderPhoneme(phoneme);
    }

    void PhonemeListModel::onPhonemeDataChanged(dspx::Phoneme *phoneme) {
        int row = findPhonemeRow(phoneme);
        if (row == -1)
            return;

        auto idx = index(row, 0);
        Q_EMIT dataChanged(idx, idx);
    }

}

#include "moc_PhonemeListModel.cpp"
