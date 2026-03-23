#ifndef DIFFSCOPE_DSPX_MODEL_PHONEMEINFO_H
#define DIFFSCOPE_DSPX_MODEL_PHONEMEINFO_H

#include <qqmlintegration.h>

#include <QObject>

#include <dspxmodel/Handle.h>

namespace opendspx {
    struct Phonemes;
}

namespace dspx {

    class Model;
    class ModelPrivate;
    class PhonemeSequence;
    class Note;

    class PhonemeInfoPrivate;

    class DSPX_MODEL_EXPORT PhonemeInfo : public QObject {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_DECLARE_PRIVATE(PhonemeInfo)
        Q_PROPERTY(PhonemeSequence *edited READ edited CONSTANT)
        Q_PROPERTY(PhonemeSequence *original READ original CONSTANT)
        Q_PROPERTY(Note *note READ note CONSTANT)

    public:
        ~PhonemeInfo() override;

        PhonemeSequence *edited() const;
        PhonemeSequence *original() const;

        Note *note() const;

        opendspx::Phonemes toOpenDspx() const;
        void fromOpenDspx(const opendspx::Phonemes &phonemeInfo);

    Q_SIGNALS:
        void noteChanged();

    private:
        friend class ModelPrivate;
        explicit PhonemeInfo(Note *note, Handle handle, Model *model);
        QScopedPointer<PhonemeInfoPrivate> d_ptr;
    };

}

#endif //DIFFSCOPE_DSPX_MODEL_PHONEMEINFO_H
