#ifndef DIFFSCOPE_SYNTH_SYNTHESISAUDIOCONTROLLER_H
#define DIFFSCOPE_SYNTH_SYNTHESISAUDIOCONTROLLER_H

#include <QHash>
#include <QObject>
#include <QPointer>
#include <QString>

namespace Core {
    class ProjectWindowInterface;
}

namespace Audio {
    class TrackAudioContext;
}

namespace dspx {
    class SingingClip;
}

namespace talcs {
    class FutureAudioSourceClipSeries;
}

namespace Synth {
    class SynthesisPiece;
}

namespace Synth::Internal {

    class SynthesisAudioController final : public QObject {
        Q_OBJECT

    public:
        explicit SynthesisAudioController(QObject *parent = nullptr);
        ~SynthesisAudioController() override;

        bool prepare(Core::ProjectWindowInterface *window, dspx::SingingClip *clip, SynthesisPiece *piece, QPointer<Audio::TrackAudioContext> &trackContext, talcs::FutureAudioSourceClipSeries *&series, QString *errorMessage = nullptr);
        bool install(Core::ProjectWindowInterface *window, dspx::SingingClip *clip, SynthesisPiece *piece, QPointer<Audio::TrackAudioContext> &trackContext, talcs::FutureAudioSourceClipSeries *&series, const QString &filePath, QString *errorMessage = nullptr);
        bool refreshRange(Core::ProjectWindowInterface *window, dspx::SingingClip *clip, SynthesisPiece *piece, Audio::TrackAudioContext *trackContext, talcs::FutureAudioSourceClipSeries *series, double sampleRate, QString *errorMessage = nullptr);
        void remove(SynthesisPiece *piece);
        void discard(SynthesisPiece *piece);
        void detachSeries(QPointer<Audio::TrackAudioContext> &trackContext, talcs::FutureAudioSourceClipSeries *&series);
        void rebindClip(dspx::SingingClip *clip, SynthesisPiece *piece);

        bool hasBinding(SynthesisPiece *piece) const;
        bool isCanceled(SynthesisPiece *piece) const;
        bool isLoaded(SynthesisPiece *piece) const;

    Q_SIGNALS:
        void statusChanged(Synth::SynthesisPiece *piece);

    private:
        struct Binding;
        void destroyBinding(Binding *binding);

        QHash<SynthesisPiece *, Binding *> m_bindings;
    };

}

#endif // DIFFSCOPE_SYNTH_SYNTHESISAUDIOCONTROLLER_H
