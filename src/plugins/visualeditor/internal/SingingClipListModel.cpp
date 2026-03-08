#include "SingingClipListModel.h"

#include <QStandardItem>

#include <dspxmodel/Clip.h>
#include <dspxmodel/ClipSequence.h>

namespace VisualEditor::Internal {

    SingingClipListModel::SingingClipListModel(QObject *parent) : QStandardItemModel(parent) {
    }

    SingingClipListModel::~SingingClipListModel() = default;

    dspx::ClipSequence *SingingClipListModel::clipSequence() const {
        return m_clipSequence;
    }

    void SingingClipListModel::setClipSequence(dspx::ClipSequence *clipSequence) {
        if (m_clipSequence == clipSequence)
            return;

        resetModel();

        m_clipSequence = clipSequence;

        if (m_clipSequence) {
            connect(m_clipSequence, &dspx::ClipSequence::itemInserted, this, &SingingClipListModel::onClipInserted);
            connect(m_clipSequence, &dspx::ClipSequence::itemRemoved, this, &SingingClipListModel::onClipRemoved);
            populateFromSequence();
        }

        Q_EMIT clipSequenceChanged();
    }

    QHash<int, QByteArray> SingingClipListModel::roleNames() const {
        static const QHash<int, QByteArray> roles{{Qt::DisplayRole, "display"}, {ClipObjectRole, "clip"}, {ClipPositionRole, "position"}};
        return roles;
    }

    void SingingClipListModel::resetModel() {
        for (int i = 0; i < rowCount(); ++i) {
            if (auto *item = this->item(i)) {
                auto clip = item->data(ClipObjectRole).value<dspx::Clip *>();
                if (clip) {
                    disconnect(clip, nullptr, this, nullptr);
                }
            }
        }

        if (m_clipSequence) {
            disconnect(m_clipSequence, nullptr, this, nullptr);
        }

        clear();
    }

    void SingingClipListModel::populateFromSequence() {
        if (!m_clipSequence)
            return;

        for (auto *clip : m_clipSequence->asRange()) {
            addClipIfSinging(clip);
        }
    }

    void SingingClipListModel::addClipIfSinging(dspx::Clip *clip) {
        if (!clip || clip->type() != dspx::Clip::Singing)
            return;

        if (itemForClip(clip))
            return;

        auto *item = new QStandardItem;
        item->setData(clip->name(), Qt::DisplayRole);
        item->setData(QVariant::fromValue(clip), ClipObjectRole);
        item->setData(clip->position(), ClipPositionRole);
        appendRow(item);

        attachClipSignals(clip);
    }

    void SingingClipListModel::removeClip(dspx::Clip *clip) {
        if (!clip)
            return;

        if (auto *item = itemForClip(clip)) {
            disconnect(clip, nullptr, this, nullptr);
            removeRow(item->row());
        }
    }

    QStandardItem *SingingClipListModel::itemForClip(const dspx::Clip *clip) const {
        if (!clip)
            return nullptr;

        for (int i = 0; i < rowCount(); ++i) {
            auto *item = this->item(i);
            if (!item)
                continue;
            auto storedClip = item->data(ClipObjectRole).value<dspx::Clip *>();
            if (storedClip == clip)
                return item;
        }
        return nullptr;
    }

    void SingingClipListModel::attachClipSignals(dspx::Clip *clip) {
        if (!clip)
            return;

        connect(clip, &dspx::Clip::nameChanged, this, [this, clip](const QString &name) {
            if (auto *item = itemForClip(clip)) {
                item->setData(name, Qt::DisplayRole);
            }
        });

        connect(clip, &dspx::Clip::positionChanged, this, [this, clip](int position) {
            if (auto *item = itemForClip(clip)) {
                item->setData(position, ClipPositionRole);
            }
        });
    }

    void SingingClipListModel::onClipInserted(dspx::Clip *clip, dspx::ClipSequence *) {
        addClipIfSinging(clip);
    }

    void SingingClipListModel::onClipRemoved(dspx::Clip *clip, dspx::ClipSequence *) {
        removeClip(clip);
    }

}

#include "moc_SingingClipListModel.cpp"
