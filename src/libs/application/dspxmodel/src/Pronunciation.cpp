#include "Pronunciation.h"

#include <QVariant>
#include <QJSEngine>

#include <opendspx/pronunciation.h>

#include <dspxmodel/private/Model_p.h>
#include <dspxmodel/ModelStrategy.h>

namespace dspx {

    class PronunciationPrivate {
        Q_DECLARE_PUBLIC(Pronunciation)
    public:
        Pronunciation *q_ptr;
        ModelPrivate *pModel;
        Handle handle;

        QString original;
        QString edited;
    };

    Pronunciation::Pronunciation(Handle handle, Model *model) : QObject(model), d_ptr(new PronunciationPrivate) {
        Q_D(Pronunciation);
        d->q_ptr = this;
        d->pModel = ModelPrivate::get(model);
        d->handle = handle;
        d->original = d->pModel->strategy->getEntityProperty(handle, ModelStrategy::P_PronunciationOriginal).toString();
        d->edited = d->pModel->strategy->getEntityProperty(handle, ModelStrategy::P_PronunciationEdited).toString();
    }

    Pronunciation::~Pronunciation() = default;

    QString Pronunciation::original() const {
        Q_D(const Pronunciation);
        return d->original;
    }

    void Pronunciation::setOriginal(const QString &original) {
        Q_D(Pronunciation);
        d->pModel->strategy->setEntityProperty(d->handle, ModelStrategy::P_PronunciationOriginal, original);
    }

    QString Pronunciation::edited() const {
        Q_D(const Pronunciation);
        return d->edited;
    }

    void Pronunciation::setEdited(const QString &edited) {
        Q_D(Pronunciation);
        d->pModel->strategy->setEntityProperty(d->handle, ModelStrategy::P_PronunciationEdited, edited);
    }

    QDspx::Pronunciation Pronunciation::toQDspx() const {
        return {
            .original = original(),
            .edited = edited()
        };
    }

    void Pronunciation::fromQDspx(const QDspx::Pronunciation &pronunciation) {
        setOriginal(pronunciation.original);
        setEdited(pronunciation.edited);
    }

    void Pronunciation::handleProxySetEntityProperty(int property, const QVariant &value) {
        Q_D(Pronunciation);
        switch (property) {
            case ModelStrategy::P_PronunciationOriginal: {
                d->original = value.toString();
                Q_EMIT originalChanged(d->original);
                break;
            }
            case ModelStrategy::P_PronunciationEdited: {
                d->edited = value.toString();
                Q_EMIT editedChanged(d->edited);
                break;
            }
            default:
                Q_UNREACHABLE();
        }
    }

}

#include "moc_Pronunciation.cpp"