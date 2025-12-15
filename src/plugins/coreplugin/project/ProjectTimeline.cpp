#include "ProjectTimeline.h"
#include "ProjectTimeline_p.h"

#include <SVSCraftCore/MusicTimeline.h>
#include <SVSCraftCore/MusicTimeSignature.h>

#include <dspxmodel/Model.h>
#include <dspxmodel/Timeline.h>
#include <dspxmodel/TempoSequence.h>
#include <dspxmodel/TimeSignatureSequence.h>
#include <dspxmodel/Tempo.h>
#include <dspxmodel/TimeSignature.h>

#include <coreplugin/DspxDocument.h>

namespace Core {

    void ProjectTimelinePrivate::handleTempoInsertedOrUpdated(dspx::Tempo *tempo) {
        Q_Q(ProjectTimeline);
        if (tempoPosMap.contains(tempo)) {
            int previousPos = tempoPosMap.value(tempo);
            auto &tempoSet = tempoMap[previousPos];
            tempoSet.remove(tempo);
            if (tempoSet.isEmpty()) {
                if (previousPos != 0)
                    musicTimeline->removeTempo(previousPos);
            } else {
                musicTimeline->setTempo(previousPos, (*tempoSet.begin())->value());
            }
        } else {
            QObject::connect(tempo, &dspx::Tempo::valueChanged, q, [=, this] {
                handleTempoInsertedOrUpdated(tempo);
            });
            QObject::connect(tempo, &dspx::Tempo::posChanged, q, [=, this] {
                handleTempoInsertedOrUpdated(tempo);
            });
        }
        tempoPosMap.insert(tempo, tempo->pos());
        auto &tempoSet = tempoMap[tempo->pos()];
        tempoSet.insert(tempo);
        musicTimeline->setTempo(tempo->pos(), (*tempoSet.begin())->value());
    }
    void ProjectTimelinePrivate::handleTempoRemoved(dspx::Tempo *tempo) {
        Q_Q(ProjectTimeline);
        if (!tempoPosMap.contains(tempo))
            return;
        int pos = tempoPosMap.value(tempo);
        auto &tempoSet = tempoMap[pos];
        tempoSet.remove(tempo);
        if (tempoSet.isEmpty()) {
            if (pos != 0)
                musicTimeline->removeTempo(pos);
        } else {
            musicTimeline->setTempo(pos, (*tempoSet.begin())->value());
        }
        tempoPosMap.remove(tempo);
        QObject::disconnect(tempo, &dspx::Tempo::valueChanged, q, nullptr);
    }
    void ProjectTimelinePrivate::handleTimeSignatureInsertedOrUpdated(dspx::TimeSignature *timeSignature) {
        Q_Q(ProjectTimeline);
        if (timeSignatureMeasureMap.contains(timeSignature)) {
            int previousMeasure = timeSignatureMeasureMap.value(timeSignature);
            auto &timeSignatureSet = timeSignatureMap[previousMeasure];
            timeSignatureSet.remove(timeSignature);
            if (timeSignatureSet.isEmpty()) {
                if (previousMeasure != 0)
                    musicTimeline->removeTimeSignature(previousMeasure);
            } else {
                auto item = *timeSignatureSet.begin();
                musicTimeline->setTimeSignature(previousMeasure, {item->numerator(), item->denominator()});
            }
        } else {
            QObject::connect(timeSignature, &dspx::TimeSignature::numeratorChanged, q, [=, this] {
                handleTimeSignatureInsertedOrUpdated(timeSignature);
            });
            QObject::connect(timeSignature, &dspx::TimeSignature::denominatorChanged, q, [=, this] {
                handleTimeSignatureInsertedOrUpdated(timeSignature);
            });
            QObject::connect(timeSignature, &dspx::TimeSignature::indexChanged, q, [=, this] {
                handleTimeSignatureInsertedOrUpdated(timeSignature);
            });
        }
        timeSignatureMeasureMap.insert(timeSignature, timeSignature->index());
        auto &timeSignatureSet = timeSignatureMap[timeSignature->index()];
        timeSignatureSet.insert(timeSignature);
        auto item = *timeSignatureSet.begin();
        musicTimeline->setTimeSignature(timeSignature->index(), {item->numerator(), item->denominator()});

    }
    void ProjectTimelinePrivate::handleTimeSignatureRemoved(dspx::TimeSignature *timeSignature) {
        Q_Q(ProjectTimeline);
        if (!timeSignatureMeasureMap.contains(timeSignature))
            return;
        int measure = timeSignatureMeasureMap.value(timeSignature);
        auto &timeSignatureSet = timeSignatureMap[measure];
        timeSignatureSet.remove(timeSignature);
        if (timeSignatureSet.isEmpty()) {
            if (measure != 0)
                musicTimeline->removeTimeSignature(measure);
        } else {
            auto item = *timeSignatureSet.begin();
            musicTimeline->setTimeSignature(measure, {item->numerator(), item->denominator()});
        }
        timeSignatureMeasureMap.remove(timeSignature);
        QObject::disconnect(timeSignature, &dspx::TimeSignature::numeratorChanged, q, nullptr);
    }

    ProjectTimeline::ProjectTimeline(DspxDocument *document, QObject *parent) : QObject(parent), d_ptr(new ProjectTimelinePrivate) {
        Q_D(ProjectTimeline);
        d->q_ptr = this;
        d->document = document;
        d->musicTimeline = new SVS::MusicTimeline(this);
        auto tempoSequence = document->model()->timeline()->tempos();
        auto timeSignatureSequence = document->model()->timeline()->timeSignatures();
        for (auto item : tempoSequence->asRange()) {
            d->handleTempoInsertedOrUpdated(item);
        }
        for (auto item : timeSignatureSequence->asRange()) {
            d->handleTimeSignatureInsertedOrUpdated(item);
        }
        connect(tempoSequence, &dspx::TempoSequence::itemInserted, this, [=, this](dspx::Tempo *tempo) {
            d->handleTempoInsertedOrUpdated(tempo);
        });
        connect(tempoSequence, &dspx::TempoSequence::itemRemoved, this, [=, this](dspx::Tempo *tempo) {
            d->handleTempoRemoved(tempo);
        });
        connect(timeSignatureSequence, &dspx::TimeSignatureSequence::itemInserted, this, [=, this](dspx::TimeSignature *timeSignature) {
            d->handleTimeSignatureInsertedOrUpdated(timeSignature);
        });
        connect(timeSignatureSequence, &dspx::TimeSignatureSequence::itemRemoved, this, [=, this](dspx::TimeSignature *timeSignature) {
            d->handleTimeSignatureRemoved(timeSignature);
        });
    }

    ProjectTimeline::~ProjectTimeline() = default;

    SVS::MusicTimeline *ProjectTimeline::musicTimeline() const {
        Q_D(const ProjectTimeline);
        return d->musicTimeline;
    }

    int ProjectTimeline::position() const {
        Q_D(const ProjectTimeline);
        return d->position;
    }
    void ProjectTimeline::setPosition(int position) {
        Q_D(ProjectTimeline);
        if (d->position != position) {
            d->position = position;
            if (position >= d->rangeHint) {
                d->rangeHint = position + 1;
                Q_EMIT rangeHintChanged(d->rangeHint);
            }
            Q_EMIT positionChanged(position);
        }
    }

    int ProjectTimeline::lastPosition() const {
        Q_D(const ProjectTimeline);
        return d->lastPosition;
    }
    void ProjectTimeline::setLastPosition(int lastPosition) {
        Q_D(ProjectTimeline);
        if (d->lastPosition != lastPosition) {
            d->lastPosition = lastPosition;
            if (lastPosition >= d->rangeHint) {
                d->rangeHint = lastPosition + 1;
                Q_EMIT rangeHintChanged(d->rangeHint);
            }
            Q_EMIT lastPositionChanged(lastPosition);
        }
    }

    int ProjectTimeline::rangeHint() const {
        Q_D(const ProjectTimeline);
        return d->rangeHint;
    }
    void ProjectTimeline::setRangeHint(int rangeHint) {
        Q_D(ProjectTimeline);
        if (d->rangeHint != rangeHint) {
            d->rangeHint = rangeHint;
            Q_EMIT rangeHintChanged(rangeHint);
        }
    }
    void ProjectTimeline::goTo(int position) {
        setPosition(position);
        setLastPosition(position);
    }

}

#include "moc_ProjectTimeline.cpp"
