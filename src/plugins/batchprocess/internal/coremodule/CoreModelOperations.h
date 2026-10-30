// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_BATCHPROCESS_COREMODELOPERATIONS_H
#define DIFFSCOPE_BATCHPROCESS_COREMODELOPERATIONS_H

#include <QJsonValue>
#include <QList>
#include <QPointer>
#include <QString>

#include <dspxmodelORM/AnchorNode.h>
#include <dspxmodelORM/AnchorNodeSequence.h>
#include <dspxmodelORM/AudioClip.h>
#include <dspxmodelORM/AudioDSP.h>
#include <dspxmodelORM/AudioDSPList.h>
#include <dspxmodelORM/Clip.h>
#include <dspxmodelORM/ClipSequence.h>
#include <dspxmodelORM/DynamicMixingAnchor.h>
#include <dspxmodelORM/DynamicMixingAnchorSequence.h>
#include <dspxmodelORM/FreeValueDataArray.h>
#include <dspxmodelORM/KeySignature.h>
#include <dspxmodelORM/KeySignatureSequence.h>
#include <dspxmodelORM/Label.h>
#include <dspxmodelORM/LabelSequence.h>
#include <dspxmodelORM/MixedSinger.h>
#include <dspxmodelORM/Model.h>
#include <dspxmodelORM/Note.h>
#include <dspxmodelORM/NoteSequence.h>
#include <dspxmodelORM/Parameter.h>
#include <dspxmodelORM/ParameterMap.h>
#include <dspxmodelORM/Phoneme.h>
#include <dspxmodelORM/PhonemeSequence.h>
#include <dspxmodelORM/Singer.h>
#include <dspxmodelORM/SingerList.h>
#include <dspxmodelORM/SingingClip.h>
#include <dspxmodelORM/SingleSinger.h>
#include <dspxmodelORM/Sources.h>
#include <dspxmodelORM/Tempo.h>
#include <dspxmodelORM/TempoSequence.h>
#include <dspxmodelORM/TimeSignature.h>
#include <dspxmodelORM/TimeSignatureSequence.h>
#include <dspxmodelORM/Track.h>
#include <dspxmodelORM/TrackList.h>
#include <dspxmodelORM/VibratoPointDataArray.h>

namespace BatchProcess::Internal {
    QString modelObjectKind(QObject *object);
    QObject *modelOwner(QObject *object);
    dspx::Model *owningModel(QObject *object);
    bool liveEntity(dspx::EntityObject *entity);
    bool attachedObject(QObject *object);
    QList<QObject *> modelItems(QObject *object);
    QList<QPointer<dspx::EntityObject>> modelEntities(dspx::Model *model);
    bool exportDSPX(QObject *object, QJsonValue *data);
    bool importDSPX(QObject *object, const QJsonValue &data, bool checked = true);
    dspx::EntityObject *allocateEntity(dspx::Model *model, const QString &kind, bool original = false);
    bool attachEntity(QObject *container, dspx::EntityObject *entity, int index, const QString &key);
    bool transferEntity(QObject *container, dspx::EntityObject *entity, int index);
    bool rotateContainer(QObject *container, int leftIndex, int middleIndex, int rightIndex);
}

#endif // DIFFSCOPE_BATCHPROCESS_COREMODELOPERATIONS_H

