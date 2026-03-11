#ifndef DIFFSCOPE_COREPLUGIN_PHONEMELISTMODEL_H
#define DIFFSCOPE_COREPLUGIN_PHONEMELISTMODEL_H

#include <QAbstractItemModel>

namespace dspx {
    class Phoneme;
    class PhonemeSequence;
}

namespace Core::Internal {

    class PhonemeListModel : public QAbstractItemModel {
        Q_OBJECT

        Q_PROPERTY(dspx::PhonemeSequence *phonemeSequence READ phonemeSequence WRITE setPhonemeSequence NOTIFY phonemeSequenceChanged)

    public:
        enum Roles {
            TokenRole = Qt::UserRole + 1,
            LanguageRole,
            StartRole,
            OnsetRole,
        };
        Q_ENUM(Roles)

        explicit PhonemeListModel(QObject *parent = nullptr);
        ~PhonemeListModel() override;

        dspx::PhonemeSequence *phonemeSequence() const;
        void setPhonemeSequence(dspx::PhonemeSequence *phonemeSequence);

        // QAbstractItemModel interface
        QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
        QModelIndex parent(const QModelIndex &child) const override;
        int rowCount(const QModelIndex &parent = QModelIndex()) const override;
        int columnCount(const QModelIndex &parent = QModelIndex()) const override;
        QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
        bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
        Qt::ItemFlags flags(const QModelIndex &index) const override;
        QHash<int, QByteArray> roleNames() const override;

    Q_SIGNALS:
        void phonemeSequenceChanged();

    private:
        void resetModel();
        void populateFromSequence();
        void attachPhonemeSignals(dspx::Phoneme *phoneme);
        void detachPhonemeSignals(dspx::Phoneme *phoneme);
        int findInsertPosition(int start) const;
        int findPhonemeRow(dspx::Phoneme *phoneme) const;
        void reorderPhoneme(dspx::Phoneme *phoneme);

        void onItemInserted(dspx::Phoneme *phoneme);
        void onItemRemoved(dspx::Phoneme *phoneme);
        void onPhonemeStartChanged(dspx::Phoneme *phoneme, int start);
        void onPhonemeDataChanged(dspx::Phoneme *phoneme);

    private:
        dspx::PhonemeSequence *m_phonemeSequence = nullptr;
        QList<dspx::Phoneme *> m_phonemes;
    };

}

#endif // DIFFSCOPE_COREPLUGIN_PHONEMELISTMODEL_H
