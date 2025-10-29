#ifndef DIFFSCOPE_DSPX_MODEL_PHONEMEINFO_H
#define DIFFSCOPE_DSPX_MODEL_PHONEMEINFO_H

#include <QObject>
#include <qqmlintegration.h>

#include <dspxmodel/Handle.h>

namespace QDspx {
    struct Phonemes;
}

namespace dspx {

    class Model;
    class ModelPrivate;
    class PhonemeList;

    class PhonemeInfoPrivate;

    class DSPX_MODEL_EXPORT PhonemeInfo : public QObject {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_DECLARE_PRIVATE(PhonemeInfo)
        Q_PROPERTY(PhonemeList *edited READ edited CONSTANT)
        Q_PROPERTY(PhonemeList *original READ original CONSTANT)

    public:
        ~PhonemeInfo() override;

        PhonemeList *edited() const;
        PhonemeList *original() const;

        QDspx::Phonemes toQDspx() const;
        void fromQDspx(const QDspx::Phonemes &phonemeInfo);

    private:
        friend class ModelPrivate;
        explicit PhonemeInfo(Handle handle, Model *model);
        QScopedPointer<PhonemeInfoPrivate> d_ptr;

    };

}

#endif //DIFFSCOPE_DSPX_MODEL_PHONEMEINFO_H