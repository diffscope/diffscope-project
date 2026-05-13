#include "AudioClip.h"

#include <QDir>

#include <opendspx/audioclip.h>

#include <dspxmodel/BusControl.h>
#include <dspxmodel/ClipTime.h>
#include <dspxmodel/ModelStrategy.h>
#include <dspxmodel/Workspace.h>
#include <dspxmodel/private/Model_p.h>
#include <dspxmodel/private/jsonutils_p.h>

namespace dspx {

    class AudioClipPrivate {
        Q_DECLARE_PUBLIC(AudioClip)
    public:
        AudioClip *q_ptr;
        ModelPrivate *pModel;
        AudioPathInfo path;
    };

    AudioClip::AudioClip(Handle handle, Model *model) : Clip(Audio, handle, model), d_ptr(new AudioClipPrivate) {
        Q_D(AudioClip);
        Q_ASSERT(model->strategy()->getEntityType(handle) == ModelStrategy::EI_AudioClip);
        d->q_ptr = this;
        d->pModel = ModelPrivate::get(model);
        d->path = d->pModel->strategy->getEntityProperty(handle, ModelStrategy::P_Path).value<AudioPathInfo>();
    }

    AudioClip::~AudioClip() = default;

    AudioPathInfo AudioClip::path() const {
        Q_D(const AudioClip);
        return d->path;
    }

    void AudioClip::setPath(const AudioPathInfo &path) {
        Q_D(AudioClip);
        d->pModel->strategy->setEntityProperty(handle(), ModelStrategy::P_Path, QVariant::fromValue(path));
    }

    static QByteArray encodeUserData(const QVariant &userData) {
        QByteArray data;
        QDataStream s(&data, QIODevice::WriteOnly);
        s.setVersion(QDataStream::Qt_5_15);
        s << userData;
        return data.toBase64();
    }

    static QVariant decodeUserData(const QByteArray &data) {
        QDataStream s(QByteArray::fromBase64(data));
        QVariant userData;
        s >> userData;
        return userData;
    }

    opendspx::AudioClip AudioClip::toOpenDspx() const {
        auto audioPathInfo = path();
        opendspx::AudioClip clip = {
            name().toStdString(),
            control()->toOpenDspx(),
            time()->toOpenDspx(),
            workspace()->toOpenDspx(),
            QDir(audioPathInfo.absoluteDir).filePath(audioPathInfo.fileName).toStdString(),
        };
        clip.workspace["diffscope"]["audio"] = nlohmann::json::object({
            {"absoluteDir", audioPathInfo.absoluteDir.toStdString()},
            {"relativeDir", audioPathInfo.relativeDir.toStdString()},
            {"fileName", audioPathInfo.fileName.toStdString()},
            {"formatEntryClassName", audioPathInfo.formatEntryClassName.toStdString()},
            {"userData", encodeUserData(audioPathInfo.userData)},
            {"sha512", audioPathInfo.sha512.toStdString()}
        });
        return clip;
    }

    void AudioClip::fromOpenDspx(const opendspx::AudioClip &clip) {
        setName(QString::fromStdString(clip.name));
        control()->fromOpenDspx(clip.control);
        time()->fromOpenDspx(clip.time);
        workspace()->fromOpenDspx(clip.workspace);
        auto diffscopeWorkspace = clip.workspace.contains("diffscope") ? JsonUtils::toQJsonValue(clip.workspace.at("diffscope")).toObject() : QJsonObject();
        if (diffscopeWorkspace.contains("audio")) {
            auto audio = diffscopeWorkspace["audio"].toObject();
            setPath({
                .absoluteDir = audio["absoluteDir"].toString(),
                .relativeDir = audio["relativeDir"].toString(),
                .fileName = audio["fileName"].toString(),
                .formatEntryClassName = audio["formatEntryClassName"].toString(),
                .userData = decodeUserData(audio["userData"].toString().toUtf8()),
                .sha512 = audio["sha512"].toString()
            });
        } else {
            auto fileInfo = QFileInfo(QString::fromStdString(clip.path));
            setPath({
                .absoluteDir = fileInfo.absolutePath(),
                .relativeDir = {},
                .fileName = fileInfo.fileName(),
                .formatEntryClassName = {},
                .userData = {},
                .sha512 = {}
            });
        }
    }

    void AudioClip::handleSetEntityProperty(int property, const QVariant &value) {
        Q_D(AudioClip);
        switch (property) {
            case ModelStrategy::P_Path: {
                d->path = value.value<AudioPathInfo>();
                Q_EMIT pathChanged(d->path);
                break;
            }
            default:
                Clip::handleSetEntityProperty(property, value);
        }
    }

}
