#ifndef DIFFSCOPE_VISUALEDITOR_SINGINGCLIPLISTMODEL_H
#define DIFFSCOPE_VISUALEDITOR_SINGINGCLIPLISTMODEL_H

#include <QStandardItemModel>

namespace dspx {
    class Clip;
    class ClipSequence;
}

namespace VisualEditor::Internal {

    class SingingClipListModel : public QStandardItemModel {
        Q_OBJECT

        Q_PROPERTY(dspx::ClipSequence *clipSequence READ clipSequence WRITE setClipSequence NOTIFY clipSequenceChanged)

    public:
        enum Roles {
            DisplayRole = Qt::DisplayRole,
            ClipObjectRole = Qt::UserRole + 1,
            ClipPositionRole,
        };
        Q_ENUM(Roles)

        explicit SingingClipListModel(QObject *parent = nullptr);
        ~SingingClipListModel() override;

        dspx::ClipSequence *clipSequence() const;
        void setClipSequence(dspx::ClipSequence *clipSequence);

        QHash<int, QByteArray> roleNames() const override;

    Q_SIGNALS:
        void clipSequenceChanged();

    private:
        void resetModel();
        void populateFromSequence();
        void addClipIfSinging(dspx::Clip *clip);
        void removeClip(dspx::Clip *clip);
        QStandardItem *itemForClip(const dspx::Clip *clip) const;
        void attachClipSignals(dspx::Clip *clip);

        void onClipInserted(dspx::Clip *clip, dspx::ClipSequence *from);
        void onClipRemoved(dspx::Clip *clip, dspx::ClipSequence *to);

    private:
        dspx::ClipSequence *m_clipSequence = nullptr;
    };

}

#endif // DIFFSCOPE_VISUALEDITOR_SINGINGCLIPLISTMODEL_H