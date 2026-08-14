#include "SynthesisDocumentWriter.h"

#include <algorithm>
#include <cmath>

#include <QSet>
#include <QVariant>

#include <coreplugin/ProjectTimeline.h>
#include <coreplugin/ProjectWindowInterface.h>

#include <dspxmodelORM/FreeValueDataArray.h>
#include <dspxmodelORM/Model.h>
#include <dspxmodelORM/Note.h>
#include <dspxmodelORM/Parameter.h>
#include <dspxmodelORM/ParameterMap.h>
#include <dspxmodelORM/Phoneme.h>
#include <dspxmodelORM/PhonemeSequence.h>
#include <dspxmodelORM/SingingClip.h>
#include <synth/SynthesisPiece.h>
#include <synth/internal/SynthesisProjectInput.h>

namespace Synth::Internal::DocumentWriter {

    void replaceOriginalPhonemes(dspx::Note *note, const QList<SynthesisPhoneme> &phonemes) {
        auto model = note->model();
        const auto old = note->originalPhonemes()->asRange();
        QList<dspx::Phoneme *> toRemove;
        for (auto phoneme : old) {
            toRemove.append(phoneme);
        }
        for (auto phoneme : toRemove) {
            model->destroyItem(phoneme);
        }
        for (const auto &source : phonemes) {
            auto phoneme = model->createOriginalPhoneme();
            phoneme->setToken(source.token);
            phoneme->setOnset(source.onset);
            phoneme->setLanguage(source.language);
            phoneme->setStart(static_cast<int>(std::round(source.start * 1000.0)));
            note->originalPhonemes()->insertItem(phoneme);
        }
    }

    void writeParameterOrigins(Core::ProjectWindowInterface *window, dspx::SingingClip *clip, SynthesisPiece *piece, const QMap<QString, SynthesisParameter> &parameters) {
        auto timeline = window->projectTimeline()->musicTimeline();
        const int firstIndex = std::max(0, static_cast<int>(std::floor(piece->position() / dspx::FreeValueDataArray::step())));
        const int lastIndex = std::max(firstIndex, static_cast<int>(std::ceil((piece->position() + piece->length()) / dspx::FreeValueDataArray::step())));
        const double pieceStartSeconds = ProjectInput::tickSeconds(timeline, clip->start() + piece->position());
        for (auto it = parameters.cbegin(); it != parameters.cend(); ++it) {
            auto modelParameter = clip->parameters()->item(it.key());
            if (!modelParameter || it->values.isEmpty() || it->sampleRate <= 0.0) {
                continue;
            }
            QList<QVariant> values;
            for (int index = firstIndex; index < lastIndex; ++index) {
                const int tick = index * dspx::FreeValueDataArray::step();
                const double offset = ProjectInput::tickSeconds(timeline, clip->start() + tick) - pieceStartSeconds;
                const int sample = std::clamp(static_cast<int>(std::round(offset * it->sampleRate)), 0, static_cast<int>(it->values.size()) - 1);
                values.append(static_cast<int>(std::round(it->values.at(sample))));
            }
            auto original = modelParameter->original();
            if (original->size() < firstIndex) {
                original->splice(original->size(), 0, QList<QVariant>(firstIndex - original->size()));
            }
            const int removed = std::min(static_cast<int>(values.size()), original->size() - firstIndex);
            original->splice(firstIndex, std::max(0, removed), values);
        }
    }

    void clearParameterOrigins(dspx::SingingClip *clip, const QList<QPair<double, double>> &ranges, const std::optional<QStringList> &parameterIds) {
        if (!clip || ranges.isEmpty() || (parameterIds && parameterIds->isEmpty())) {
            return;
        }
        const QSet<QString> selected = parameterIds ? QSet<QString>(parameterIds->cbegin(), parameterIds->cend()) : QSet<QString>{};
        const auto keys = clip->parameters()->keys();
        for (const auto &key : keys) {
            if (parameterIds && !selected.contains(key)) {
                continue;
            }
            auto parameter = clip->parameters()->item(key);
            auto original = parameter ? parameter->original() : nullptr;
            if (!original || original->size() == 0) {
                continue;
            }
            for (const auto &[position, length] : ranges) {
                const int firstIndex = std::max(0, static_cast<int>(std::floor(position / dspx::FreeValueDataArray::step())));
                const int lastIndex = std::max(firstIndex, static_cast<int>(std::ceil((position + length) / dspx::FreeValueDataArray::step())));
                if (firstIndex >= original->size()) {
                    continue;
                }
                const int count = std::min(lastIndex - firstIndex, original->size() - firstIndex);
                if (count > 0) {
                    original->splice(firstIndex, count, QList<QVariant>(count));
                }
            }
        }
    }

}
