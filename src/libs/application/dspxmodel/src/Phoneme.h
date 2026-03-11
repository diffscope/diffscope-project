#ifndef DIFFSCOPE_DSPX_MODEL_PHONEME_H
#define DIFFSCOPE_DSPX_MODEL_PHONEME_H

#include <qqmlintegration.h>

#include <dspxmodel/EntityObject.h>

namespace QDspx {
    struct Phoneme;
}

namespace dspx {

    class PhonemeSequence;
    class PhonemePrivate;

    class DSPX_MODEL_EXPORT Phoneme : public EntityObject {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_DECLARE_PRIVATE(Phoneme);
        Q_PROPERTY(QString language READ language WRITE setLanguage NOTIFY languageChanged)
        Q_PROPERTY(int start READ start WRITE setStart NOTIFY startChanged)
        Q_PROPERTY(QString token READ token WRITE setToken NOTIFY tokenChanged)
        Q_PROPERTY(bool onset READ onset WRITE setOnset NOTIFY onsetChanged)
        Q_PROPERTY(PhonemeSequence *phonemeSequence READ phonemeSequence NOTIFY phonemeSequenceChanged)
        Q_PROPERTY(Phoneme *previousItem READ previousItem NOTIFY previousItemChanged)
        Q_PROPERTY(Phoneme *nextItem READ nextItem NOTIFY nextItemChanged)

    public:
        ~Phoneme() override;

        QString language() const;
        void setLanguage(const QString &language);

        int start() const;
        void setStart(int start);

        QString token() const;
        void setToken(const QString &token);

        bool onset() const;
        void setOnset(bool onset);

        PhonemeSequence *phonemeSequence() const;

        Phoneme *previousItem() const;
        Phoneme *nextItem() const;

        QDspx::Phoneme toQDspx() const;
        void fromQDspx(const QDspx::Phoneme &phoneme);

    Q_SIGNALS:
        void languageChanged(const QString &language);
        void startChanged(int start);
        void tokenChanged(const QString &token);
        void onsetChanged(bool onset);
        void phonemeSequenceChanged();
        void previousItemChanged();
        void nextItemChanged();

    protected:
        void handleSetEntityProperty(int property, const QVariant &value) override;

    private:
        friend class ModelPrivate;
        explicit Phoneme(Handle handle, Model *model);
        QScopedPointer<PhonemePrivate> d_ptr;
    };

}

#endif //DIFFSCOPE_DSPX_MODEL_PHONEME_H
