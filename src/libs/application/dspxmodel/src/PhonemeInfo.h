#ifndef DIFFSCOPE_DSPX_MODEL_PHONEMEINFO_H
#define DIFFSCOPE_DSPX_MODEL_PHONEMEINFO_H

#include <qqmlintegration.h>

#include <QObject>

#include <dspxmodel/Handle.h>

namespace QDspx {
    struct Phonemes;
}

namespace dspx {

    class Model;
    class ModelPrivate;
    class PhonemeList;
    class Note;

    class PhonemeInfoPrivate;

    class DSPX_MODEL_EXPORT PhonemeInfo : public QObject {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_DECLARE_PRIVATE(PhonemeInfo)
        Q_PROPERTY(PhonemeList *edited READ edited CONSTANT)
        Q_PROPERTY(PhonemeList *original READ original CONSTANT)
        Q_PROPERTY(Note *note READ note CONSTANT)

    public:
        ~PhonemeInfo() override;

        PhonemeList *edited() const;
        PhonemeList *original() const;

        Note *note() const;

        QDspx::Phonemes toQDspx() const;
        void fromQDspx(const QDspx::Phonemes &phonemeInfo);

    Q_SIGNALS:
        void noteChanged();

    private:
        friend class ModelPrivate;
        explicit PhonemeInfo(Note *note, Handle handle, Model *model);
        QScopedPointer<PhonemeInfoPrivate> d_ptr;
    };

}

#endif //DIFFSCOPE_DSPX_MODEL_PHONEMEINFO_H
