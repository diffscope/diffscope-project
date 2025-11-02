#include "Phoneme.h"

#include <QJSEngine>
#include <QVariant>

#include <opendspx/phoneme.h>

#include <dspxmodel/Model.h>
#include <dspxmodel/ModelStrategy.h>

namespace dspx {

    class PhonemePrivate {
        Q_DECLARE_PUBLIC(Phoneme)
    public:
        Phoneme *q_ptr;
        QString language;
        int start;
        QString token;
        bool onset;
    };

    Phoneme::Phoneme(Handle handle, Model *model) : EntityObject(handle, model), d_ptr(new PhonemePrivate) {
        Q_D(Phoneme);
        Q_ASSERT(model->strategy()->getEntityType(handle) == ModelStrategy::EI_Phoneme);
        d->q_ptr = this;
        d->language = model->strategy()->getEntityProperty(handle, ModelStrategy::P_Language).toString();
        d->start = model->strategy()->getEntityProperty(handle, ModelStrategy::P_Position).toInt();
        d->token = model->strategy()->getEntityProperty(handle, ModelStrategy::P_Text).toString();
        d->onset = model->strategy()->getEntityProperty(handle, ModelStrategy::P_Onset).toBool();
    }

    Phoneme::~Phoneme() = default;

    QString Phoneme::language() const {
        Q_D(const Phoneme);
        return d->language;
    }

    void Phoneme::setLanguage(const QString &language) {
        Q_D(Phoneme);
        model()->strategy()->setEntityProperty(handle(), ModelStrategy::P_Language, language);
    }

    int Phoneme::start() const {
        Q_D(const Phoneme);
        return d->start;
    }

    void Phoneme::setStart(int start) {
        Q_D(Phoneme);
        model()->strategy()->setEntityProperty(handle(), ModelStrategy::P_Position, start);
    }

    QString Phoneme::token() const {
        Q_D(const Phoneme);
        return d->token;
    }

    void Phoneme::setToken(const QString &token) {
        Q_D(Phoneme);
        model()->strategy()->setEntityProperty(handle(), ModelStrategy::P_Text, token);
    }

    bool Phoneme::onset() const {
        Q_D(const Phoneme);
        return d->onset;
    }

    void Phoneme::setOnset(bool onset) {
        Q_D(Phoneme);
        model()->strategy()->setEntityProperty(handle(), ModelStrategy::P_Onset, onset);
    }

    QDspx::Phoneme Phoneme::toQDspx() const {
        return {
            .language = language(),
            .token = token(),
            .start = start(),
            .onset = onset()
        };
    }

    void Phoneme::fromQDspx(const QDspx::Phoneme &phoneme) {
        setLanguage(phoneme.language);
        setToken(phoneme.token);
        setStart(phoneme.start);
        setOnset(phoneme.onset);
    }

    void Phoneme::handleSetEntityProperty(int property, const QVariant &value) {
        Q_D(Phoneme);
        switch (property) {
            case ModelStrategy::P_Language: {
                d->language = value.toString();
                Q_EMIT languageChanged(d->language);
                break;
            }
            case ModelStrategy::P_Position: {
                d->start = value.toInt();
                Q_EMIT startChanged(d->start);
                break;
            }
            case ModelStrategy::P_Text: {
                d->token = value.toString();
                Q_EMIT tokenChanged(d->token);
                break;
            }
            case ModelStrategy::P_Onset: {
                d->onset = value.toBool();
                Q_EMIT onsetChanged(d->onset);
                break;
            }
            default:
                Q_UNREACHABLE();
        }
    }

}

#include "moc_Phoneme.cpp"
