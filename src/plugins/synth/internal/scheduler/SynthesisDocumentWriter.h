// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_SYNTH_SYNTHESISDOCUMENTWRITER_H
#define DIFFSCOPE_SYNTH_SYNTHESISDOCUMENTWRITER_H

#include <optional>

#include <QList>
#include <QMap>
#include <QPair>
#include <QStringList>

#include <synth/SynthesisModel.h>

namespace Core {
    class ProjectWindowInterface;
}

namespace dspx {
    class Note;
    class SingingClip;
}

namespace Synth {
    class SynthesisPiece;
}

namespace Synth::Internal::DocumentWriter {

    void replaceOriginalPhonemes(dspx::Note *note, const QList<SynthesisPhoneme> &phonemes);
    void writeParameterOrigins(Core::ProjectWindowInterface *window, dspx::SingingClip *clip, SynthesisPiece *piece, const QMap<QString, SynthesisParameter> &parameters);
    void clearParameterOrigins(dspx::SingingClip *clip, const QList<QPair<double, double>> &ranges, const std::optional<QStringList> &parameterIds);

}

#endif // DIFFSCOPE_SYNTH_SYNTHESISDOCUMENTWRITER_H
