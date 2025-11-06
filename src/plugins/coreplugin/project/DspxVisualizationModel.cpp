#include "DspxVisualizationModel_p.h"

#include <opendspx/model.h>
#include <opendspx/content.h>
#include <opendspx/global.h>
#include <opendspx/master.h>
#include <opendspx/buscontrol.h>
#include <opendspx/timeline.h>
#include <opendspx/label.h>
#include <opendspx/tempo.h>
#include <opendspx/timesignature.h>
#include <opendspx/track.h>
#include <opendspx/trackcontrol.h>
#include <opendspx/workspace.h>
#include <opendspx/sources.h>
#include <opendspx/clip.h>
#include <opendspx/audioclip.h>
#include <opendspx/singingclip.h>
#include <opendspx/cliptime.h>
#include <opendspx/note.h>
#include <opendspx/pronunciation.h>
#include <opendspx/phonemes.h>
#include <opendspx/phoneme.h>
#include <opendspx/vibrato.h>
#include <opendspx/vibratopoints.h>
#include <opendspx/controlpoint.h>
#include <opendspx/params.h>
#include <opendspx/param.h>
#include <opendspx/paramcurve.h>
#include <opendspx/paramcurveanchor.h>
#include <opendspx/paramcurvefree.h>
#include <opendspx/anchornode.h>

namespace Core {

    DspxVisualizationModel::DspxVisualizationModel(QObject *parent) : QStandardItemModel(parent) {
    }

    DspxVisualizationModel::~DspxVisualizationModel() = default;

    void DspxVisualizationModel::generate(const QDspx::Model &model) {
        clear();
        appendRow(createItem(model.content));
    }

    template <>
    QStandardItem * DspxVisualizationModel::createItem(const QDspx::Content &entity) const {
        auto item = new QStandardItem(tr("<Content>"));
        item->setEditable(false);
        item->appendRow(createItem(entity.global));
        item->appendRow(createItem(entity.master));
        item->appendRow(createItem(entity.timeline));
        auto trackListItem = new QStandardItem(tr("<Track list>"));
        trackListItem->setEditable(false);
        for (const auto &track : entity.tracks) {
            trackListItem->appendRow(createItem(track));
        }
        item->appendRow(trackListItem);
        item->appendRow(createItem(entity.workspace));
        return item;
    }

    template <>
    QStandardItem * DspxVisualizationModel::createItem(const QDspx::Global &entity) const {
        auto item = new QStandardItem(tr("<Global>"));
        item->setEditable(false);
        
        auto authorItem = new QStandardItem(tr("Author"));
        authorItem->setEditable(false);
        auto authorValueItem = new QStandardItem();
        authorValueItem->setData(entity.author, Qt::DisplayRole);
        item->appendRow({authorItem, authorValueItem});
        
        auto nameItem = new QStandardItem(tr("Name"));
        nameItem->setEditable(false);
        auto nameValueItem = new QStandardItem();
        nameValueItem->setData(entity.name, Qt::DisplayRole);
        item->appendRow({nameItem, nameValueItem});
        
        auto centShiftItem = new QStandardItem(tr("Cent shift"));
        centShiftItem->setEditable(false);
        auto centShiftValueItem = new QStandardItem();
        centShiftValueItem->setData(entity.centShift, Qt::DisplayRole);
        item->appendRow({centShiftItem, centShiftValueItem});
        
        auto editorIdItem = new QStandardItem(tr("Editor identifier"));
        editorIdItem->setEditable(false);
        auto editorIdValueItem = new QStandardItem();
        editorIdValueItem->setData(entity.editorId, Qt::DisplayRole);
        item->appendRow({editorIdItem, editorIdValueItem});
        
        auto editorNameItem = new QStandardItem(tr("Editor name"));
        editorNameItem->setEditable(false);
        auto editorNameValueItem = new QStandardItem();
        editorNameValueItem->setData(entity.editorName, Qt::DisplayRole);
        item->appendRow({editorNameItem, editorNameValueItem});
        
        return item;
    }

    template <>
    QStandardItem * DspxVisualizationModel::createItem(const QDspx::BusControl &entity) const {
        auto item = new QStandardItem(tr("<Bus control>"));
        item->setEditable(false);
        
        auto gainItem = new QStandardItem(tr("Gain"));
        gainItem->setEditable(false);
        auto gainValueItem = new QStandardItem();
        gainValueItem->setData(entity.gain, Qt::DisplayRole);
        item->appendRow({gainItem, gainValueItem});
        
        auto panItem = new QStandardItem(tr("Pan"));
        panItem->setEditable(false);
        auto panValueItem = new QStandardItem();
        panValueItem->setData(entity.pan, Qt::DisplayRole);
        item->appendRow({panItem, panValueItem});
        
        auto muteItem = new QStandardItem(tr("Mute"));
        muteItem->setEditable(false);
        auto muteValueItem = new QStandardItem();
        muteValueItem->setData(entity.mute, Qt::DisplayRole);
        item->appendRow({muteItem, muteValueItem});
        
        return item;
    }

    template <>
    QStandardItem * DspxVisualizationModel::createItem(const QDspx::TrackControl &entity) const {
        auto item = new QStandardItem(tr("<Track control>"));
        item->setEditable(false);
        
        auto gainItem = new QStandardItem(tr("Gain"));
        gainItem->setEditable(false);
        auto gainValueItem = new QStandardItem();
        gainValueItem->setData(entity.gain, Qt::DisplayRole);
        item->appendRow({gainItem, gainValueItem});
        
        auto panItem = new QStandardItem(tr("Pan"));
        panItem->setEditable(false);
        auto panValueItem = new QStandardItem();
        panValueItem->setData(entity.pan, Qt::DisplayRole);
        item->appendRow({panItem, panValueItem});
        
        auto muteItem = new QStandardItem(tr("Mute"));
        muteItem->setEditable(false);
        auto muteValueItem = new QStandardItem();
        muteValueItem->setData(entity.mute, Qt::DisplayRole);
        item->appendRow({muteItem, muteValueItem});
        
        auto soloItem = new QStandardItem(tr("Solo"));
        soloItem->setEditable(false);
        auto soloValueItem = new QStandardItem();
        soloValueItem->setData(entity.solo, Qt::DisplayRole);
        item->appendRow({soloItem, soloValueItem});
        
        return item;
    }

    template <>
    QStandardItem * DspxVisualizationModel::createItem(const QDspx::Label &entity) const {
        auto item = new QStandardItem(tr("<Label>"));
        item->setEditable(false);
        
        auto posItem = new QStandardItem(tr("Position"));
        posItem->setEditable(false);
        auto posValueItem = new QStandardItem();
        posValueItem->setData(entity.pos, Qt::DisplayRole);
        item->appendRow({posItem, posValueItem});
        
        auto textItem = new QStandardItem(tr("Text"));
        textItem->setEditable(false);
        auto textValueItem = new QStandardItem();
        textValueItem->setData(entity.text, Qt::DisplayRole);
        item->appendRow({textItem, textValueItem});
        
        return item;
    }

    template <>
    QStandardItem * DspxVisualizationModel::createItem(const QDspx::Tempo &entity) const {
        auto item = new QStandardItem(tr("<Tempo>"));
        item->setEditable(false);
        
        auto posItem = new QStandardItem(tr("Position"));
        posItem->setEditable(false);
        auto posValueItem = new QStandardItem();
        posValueItem->setData(entity.pos, Qt::DisplayRole);
        item->appendRow({posItem, posValueItem});
        
        auto valueItem = new QStandardItem(tr("Value"));
        valueItem->setEditable(false);
        auto valueValueItem = new QStandardItem();
        valueValueItem->setData(entity.value, Qt::DisplayRole);
        item->appendRow({valueItem, valueValueItem});
        
        return item;
    }

    template <>
    QStandardItem * DspxVisualizationModel::createItem(const QDspx::TimeSignature &entity) const {
        auto item = new QStandardItem(tr("<Time signature>"));
        item->setEditable(false);
        
        auto indexItem = new QStandardItem(tr("Index"));
        indexItem->setEditable(false);
        auto indexValueItem = new QStandardItem();
        indexValueItem->setData(entity.index, Qt::DisplayRole);
        item->appendRow({indexItem, indexValueItem});
        
        auto numeratorItem = new QStandardItem(tr("Numerator"));
        numeratorItem->setEditable(false);
        auto numeratorValueItem = new QStandardItem();
        numeratorValueItem->setData(entity.numerator, Qt::DisplayRole);
        item->appendRow({numeratorItem, numeratorValueItem});
        
        auto denominatorItem = new QStandardItem(tr("Denominator"));
        denominatorItem->setEditable(false);
        auto denominatorValueItem = new QStandardItem();
        denominatorValueItem->setData(entity.denominator, Qt::DisplayRole);
        item->appendRow({denominatorItem, denominatorValueItem});
        
        return item;
    }

    template <>
    QStandardItem * DspxVisualizationModel::createItem(const QDspx::ClipTime &entity) const {
        auto item = new QStandardItem(tr("<Clip time>"));
        item->setEditable(false);
        
        auto startItem = new QStandardItem(tr("Start"));
        startItem->setEditable(false);
        auto startValueItem = new QStandardItem();
        startValueItem->setData(entity.start, Qt::DisplayRole);
        item->appendRow({startItem, startValueItem});
        
        auto lengthItem = new QStandardItem(tr("Length"));
        lengthItem->setEditable(false);
        auto lengthValueItem = new QStandardItem();
        lengthValueItem->setData(entity.length, Qt::DisplayRole);
        item->appendRow({lengthItem, lengthValueItem});
        
        auto clipStartItem = new QStandardItem(tr("Clip start"));
        clipStartItem->setEditable(false);
        auto clipStartValueItem = new QStandardItem();
        clipStartValueItem->setData(entity.clipStart, Qt::DisplayRole);
        item->appendRow({clipStartItem, clipStartValueItem});
        
        auto clipLenItem = new QStandardItem(tr("Clip length"));
        clipLenItem->setEditable(false);
        auto clipLenValueItem = new QStandardItem();
        clipLenValueItem->setData(entity.clipLen, Qt::DisplayRole);
        item->appendRow({clipLenItem, clipLenValueItem});
        
        return item;
    }

    template <>
    QStandardItem * DspxVisualizationModel::createItem(const QDspx::Pronunciation &entity) const {
        auto item = new QStandardItem(tr("<Pronunciation>"));
        item->setEditable(false);
        
        auto originalItem = new QStandardItem(tr("Original"));
        originalItem->setEditable(false);
        auto originalValueItem = new QStandardItem();
        originalValueItem->setData(entity.original, Qt::DisplayRole);
        item->appendRow({originalItem, originalValueItem});
        
        auto editedItem = new QStandardItem(tr("Edited"));
        editedItem->setEditable(false);
        auto editedValueItem = new QStandardItem();
        editedValueItem->setData(entity.edited, Qt::DisplayRole);
        item->appendRow({editedItem, editedValueItem});
        
        return item;
    }

    template <>
    QStandardItem * DspxVisualizationModel::createItem(const QDspx::Phoneme &entity) const {
        auto item = new QStandardItem(tr("<Phoneme>"));
        item->setEditable(false);
        
        auto languageItem = new QStandardItem(tr("Language"));
        languageItem->setEditable(false);
        auto languageValueItem = new QStandardItem();
        languageValueItem->setData(entity.language, Qt::DisplayRole);
        item->appendRow({languageItem, languageValueItem});
        
        auto tokenItem = new QStandardItem(tr("Token"));
        tokenItem->setEditable(false);
        auto tokenValueItem = new QStandardItem();
        tokenValueItem->setData(entity.token, Qt::DisplayRole);
        item->appendRow({tokenItem, tokenValueItem});
        
        auto startItem = new QStandardItem(tr("Start"));
        startItem->setEditable(false);
        auto startValueItem = new QStandardItem();
        startValueItem->setData(entity.start, Qt::DisplayRole);
        item->appendRow({startItem, startValueItem});
        
        auto onsetItem = new QStandardItem(tr("Onset"));
        onsetItem->setEditable(false);
        auto onsetValueItem = new QStandardItem();
        onsetValueItem->setData(entity.onset, Qt::DisplayRole);
        item->appendRow({onsetItem, onsetValueItem});
        
        return item;
    }

    template <>
    QStandardItem * DspxVisualizationModel::createItem(const QDspx::ControlPoint &entity) const {
        auto item = new QStandardItem(tr("<Control point>"));
        item->setEditable(false);
        
        auto xItem = new QStandardItem(tr("X"));
        xItem->setEditable(false);
        auto xValueItem = new QStandardItem();
        xValueItem->setData(entity.x, Qt::DisplayRole);
        item->appendRow({xItem, xValueItem});
        
        auto yItem = new QStandardItem(tr("Y"));
        yItem->setEditable(false);
        auto yValueItem = new QStandardItem();
        yValueItem->setData(entity.y, Qt::DisplayRole);
        item->appendRow({yItem, yValueItem});
        
        return item;
    }

    template <>
    QStandardItem * DspxVisualizationModel::createItem(const QDspx::AnchorNode &entity) const {
        auto item = new QStandardItem(tr("<Anchor node>"));
        item->setEditable(false);
        
        auto interpItem = new QStandardItem(tr("Interpolation"));
        interpItem->setEditable(false);
        auto interpValueItem = new QStandardItem();
        interpValueItem->setData(static_cast<int>(entity.interp), Qt::DisplayRole);
        item->appendRow({interpItem, interpValueItem});
        
        auto xItem = new QStandardItem(tr("X"));
        xItem->setEditable(false);
        auto xValueItem = new QStandardItem();
        xValueItem->setData(entity.x, Qt::DisplayRole);
        item->appendRow({xItem, xValueItem});
        
        auto yItem = new QStandardItem(tr("Y"));
        yItem->setEditable(false);
        auto yValueItem = new QStandardItem();
        yValueItem->setData(entity.y, Qt::DisplayRole);
        item->appendRow({yItem, yValueItem});
        
        return item;
    }

    template <>
    QStandardItem * DspxVisualizationModel::createItem(const QDspx::Timeline &entity) const {
        auto item = new QStandardItem(tr("<Timeline>"));
        item->setEditable(false);
        
        auto labelsItem = new QStandardItem(tr("<Label list>"));
        labelsItem->setEditable(false);
        for (const auto &label : entity.labels) {
            labelsItem->appendRow(createItem(label));
        }
        item->appendRow(labelsItem);
        
        auto temposItem = new QStandardItem(tr("<Tempo list>"));
        temposItem->setEditable(false);
        for (const auto &tempo : entity.tempos) {
            temposItem->appendRow(createItem(tempo));
        }
        item->appendRow(temposItem);
        
        auto timeSignaturesItem = new QStandardItem(tr("<Time signature list>"));
        timeSignaturesItem->setEditable(false);
        for (const auto &timeSignature : entity.timeSignatures) {
            timeSignaturesItem->appendRow(createItem(timeSignature));
        }
        item->appendRow(timeSignaturesItem);
        
        return item;
    }

    template <>
    QStandardItem * DspxVisualizationModel::createItem(const QDspx::Phonemes &entity) const {
        auto item = new QStandardItem(tr("<Phonemes>"));
        item->setEditable(false);
        
        auto originalItem = new QStandardItem(tr("<Original phoneme list>"));
        originalItem->setEditable(false);
        for (const auto &phoneme : entity.original) {
            originalItem->appendRow(createItem(phoneme));
        }
        item->appendRow(originalItem);
        
        auto editedItem = new QStandardItem(tr("<Edited phoneme list>"));
        editedItem->setEditable(false);
        for (const auto &phoneme : entity.edited) {
            editedItem->appendRow(createItem(phoneme));
        }
        item->appendRow(editedItem);
        
        return item;
    }

    template <>
    QStandardItem * DspxVisualizationModel::createItem(const QDspx::VibratoPoints &entity) const {
        auto item = new QStandardItem(tr("<Vibrato points>"));
        item->setEditable(false);
        
        auto ampItem = new QStandardItem(tr("<Amplitude control point list>"));
        ampItem->setEditable(false);
        for (const auto &point : entity.amp) {
            ampItem->appendRow(createItem(point));
        }
        item->appendRow(ampItem);
        
        auto freqItem = new QStandardItem(tr("<Frequency control point list>"));
        freqItem->setEditable(false);
        for (const auto &point : entity.freq) {
            freqItem->appendRow(createItem(point));
        }
        item->appendRow(freqItem);
        
        return item;
    }

    template <>
    QStandardItem * DspxVisualizationModel::createItem(const QDspx::Vibrato &entity) const {
        auto item = new QStandardItem(tr("<Vibrato>"));
        item->setEditable(false);
        
        auto startItem = new QStandardItem(tr("Start"));
        startItem->setEditable(false);
        auto startValueItem = new QStandardItem();
        startValueItem->setData(entity.start, Qt::DisplayRole);
        item->appendRow({startItem, startValueItem});
        
        auto endItem = new QStandardItem(tr("End"));
        endItem->setEditable(false);
        auto endValueItem = new QStandardItem();
        endValueItem->setData(entity.end, Qt::DisplayRole);
        item->appendRow({endItem, endValueItem});
        
        auto ampItem = new QStandardItem(tr("Amplitude"));
        ampItem->setEditable(false);
        auto ampValueItem = new QStandardItem();
        ampValueItem->setData(entity.amp, Qt::DisplayRole);
        item->appendRow({ampItem, ampValueItem});
        
        auto freqItem = new QStandardItem(tr("Frequency"));
        freqItem->setEditable(false);
        auto freqValueItem = new QStandardItem();
        freqValueItem->setData(entity.freq, Qt::DisplayRole);
        item->appendRow({freqItem, freqValueItem});
        
        auto phaseItem = new QStandardItem(tr("Phase"));
        phaseItem->setEditable(false);
        auto phaseValueItem = new QStandardItem();
        phaseValueItem->setData(entity.phase, Qt::DisplayRole);
        item->appendRow({phaseItem, phaseValueItem});
        
        auto offsetItem = new QStandardItem(tr("Offset"));
        offsetItem->setEditable(false);
        auto offsetValueItem = new QStandardItem();
        offsetValueItem->setData(entity.offset, Qt::DisplayRole);
        item->appendRow({offsetItem, offsetValueItem});
        
        item->appendRow(createItem(entity.points));
        
        return item;
    }

    template <>
    QStandardItem * DspxVisualizationModel::createItem(const QDspx::Param &entity) const {
        auto item = new QStandardItem(tr("<Parameter>"));
        item->setEditable(false);
        
        auto originalItem = new QStandardItem(tr("<Original curve list>"));
        originalItem->setEditable(false);
        for (const auto &curve : entity.original) {
            originalItem->appendRow(createItem(*curve));
        }
        item->appendRow(originalItem);
        
        auto transformItem = new QStandardItem(tr("<Transform curve list>"));
        transformItem->setEditable(false);
        for (const auto &curve : entity.transform) {
            transformItem->appendRow(createItem(*curve));
        }
        item->appendRow(transformItem);
        
        auto editedItem = new QStandardItem(tr("<Edited curve list>"));
        editedItem->setEditable(false);
        for (const auto &curve : entity.edited) {
            editedItem->appendRow(createItem(*curve));
        }
        item->appendRow(editedItem);
        
        return item;
    }

    template <>
    QStandardItem * DspxVisualizationModel::createItem(const QDspx::Note &entity) const {
        auto item = new QStandardItem(tr("<Note>"));
        item->setEditable(false);
        
        auto posItem = new QStandardItem(tr("Position"));
        posItem->setEditable(false);
        auto posValueItem = new QStandardItem();
        posValueItem->setData(entity.pos, Qt::DisplayRole);
        item->appendRow({posItem, posValueItem});
        
        auto lengthItem = new QStandardItem(tr("Length"));
        lengthItem->setEditable(false);
        auto lengthValueItem = new QStandardItem();
        lengthValueItem->setData(entity.length, Qt::DisplayRole);
        item->appendRow({lengthItem, lengthValueItem});
        
        auto keyNumItem = new QStandardItem(tr("Key number"));
        keyNumItem->setEditable(false);
        auto keyNumValueItem = new QStandardItem();
        keyNumValueItem->setData(entity.keyNum, Qt::DisplayRole);
        item->appendRow({keyNumItem, keyNumValueItem});
        
        auto centShiftItem = new QStandardItem(tr("Cent shift"));
        centShiftItem->setEditable(false);
        auto centShiftValueItem = new QStandardItem();
        centShiftValueItem->setData(entity.centShift, Qt::DisplayRole);
        item->appendRow({centShiftItem, centShiftValueItem});
        
        auto languageItem = new QStandardItem(tr("Language"));
        languageItem->setEditable(false);
        auto languageValueItem = new QStandardItem();
        languageValueItem->setData(entity.language, Qt::DisplayRole);
        item->appendRow({languageItem, languageValueItem});
        
        auto lyricItem = new QStandardItem(tr("Lyric"));
        lyricItem->setEditable(false);
        auto lyricValueItem = new QStandardItem();
        lyricValueItem->setData(entity.lyric, Qt::DisplayRole);
        item->appendRow({lyricItem, lyricValueItem});
        
        item->appendRow(createItem(entity.pronunciation));
        item->appendRow(createItem(entity.phonemes));
        item->appendRow(createItem(entity.vibrato));
        item->appendRow(createItem(entity.workspace));
        
        return item;
    }

    template <>
    QStandardItem * DspxVisualizationModel::createItem(const QDspx::Clip &entity) const {
        switch (entity.type) {
            case QDspx::Clip::Audio:
                return createItem(static_cast<const QDspx::AudioClip&>(entity));
            case QDspx::Clip::Singing:
                return createItem(static_cast<const QDspx::SingingClip&>(entity));
            default:
                break;
        }
        // Fallback
        auto item = new QStandardItem(tr("<Clip>"));
        item->setEditable(false);
        return item;
    }

    template <>
    QStandardItem * DspxVisualizationModel::createItem(const QDspx::AudioClip &entity) const {
        auto item = new QStandardItem(tr("<Audio clip>"));
        item->setEditable(false);
        
        auto nameItem = new QStandardItem(tr("Name"));
        nameItem->setEditable(false);
        auto nameValueItem = new QStandardItem();
        nameValueItem->setData(entity.name, Qt::DisplayRole);
        item->appendRow({nameItem, nameValueItem});
        
        auto pathItem = new QStandardItem(tr("Path"));
        pathItem->setEditable(false);
        auto pathValueItem = new QStandardItem();
        pathValueItem->setData(entity.path, Qt::DisplayRole);
        item->appendRow({pathItem, pathValueItem});
        
        item->appendRow(createItem(entity.control));
        item->appendRow(createItem(entity.time));
        item->appendRow(createItem(entity.workspace));
        
        return item;
    }

    template <>
    QStandardItem * DspxVisualizationModel::createItem(const QDspx::SingingClip &entity) const {
        auto item = new QStandardItem(tr("<Singing clip>"));
        item->setEditable(false);
        
        auto nameItem = new QStandardItem(tr("Name"));
        nameItem->setEditable(false);
        auto nameValueItem = new QStandardItem();
        nameValueItem->setData(entity.name, Qt::DisplayRole);
        item->appendRow({nameItem, nameValueItem});
        
        item->appendRow(createItem(entity.control));
        item->appendRow(createItem(entity.time));
        
        auto noteListItem = new QStandardItem(tr("<Note list>"));
        noteListItem->setEditable(false);
        for (const auto &note : entity.notes) {
            noteListItem->appendRow(createItem(note));
        }
        item->appendRow(noteListItem);
        
        auto paramsItem = new QStandardItem(tr("<Parameters>"));
        paramsItem->setEditable(false);
        for (auto it = entity.params.begin(); it != entity.params.end(); ++it) {
            auto paramItem = new QStandardItem(it.key());
            paramItem->setEditable(false);
            paramItem->appendRow(createItem(it.value()));
            paramsItem->appendRow(paramItem);
        }
        item->appendRow(paramsItem);
        
        item->appendRow(createItem(entity.sources));
        item->appendRow(createItem(entity.workspace));
        
        return item;
    }

    template <>
    QStandardItem * DspxVisualizationModel::createItem(const QDspx::ParamCurve &entity) const {
        switch (entity.type) {
            case QDspx::ParamCurve::Anchor:
                return createItem(static_cast<const QDspx::ParamCurveAnchor&>(entity));
            case QDspx::ParamCurve::Free:
                return createItem(static_cast<const QDspx::ParamCurveFree&>(entity));
            default:
                break;
        }
        // Fallback
        auto item = new QStandardItem(tr("<Parameter curve>"));
        item->setEditable(false);
        return item;
    }

    template <>
    QStandardItem * DspxVisualizationModel::createItem(const QDspx::ParamCurveAnchor &entity) const {
        auto item = new QStandardItem(tr("<Parameter curve anchor>"));
        item->setEditable(false);
        
        auto startItem = new QStandardItem(tr("Start"));
        startItem->setEditable(false);
        auto startValueItem = new QStandardItem();
        startValueItem->setData(entity.start, Qt::DisplayRole);
        item->appendRow({startItem, startValueItem});
        
        auto nodeListItem = new QStandardItem(tr("<Anchor node list>"));
        nodeListItem->setEditable(false);
        for (const auto &node : entity.nodes) {
            nodeListItem->appendRow(createItem(node));
        }
        item->appendRow(nodeListItem);
        
        return item;
    }

    template <>
    QStandardItem * DspxVisualizationModel::createItem(const QDspx::ParamCurveFree &entity) const {
        auto item = new QStandardItem(tr("<Parameter curve free>"));
        item->setEditable(false);
        
        auto startItem = new QStandardItem(tr("Start"));
        startItem->setEditable(false);
        auto startValueItem = new QStandardItem();
        startValueItem->setData(entity.start, Qt::DisplayRole);
        item->appendRow({startItem, startValueItem});
        
        auto stepItem = new QStandardItem(tr("Step"));
        stepItem->setEditable(false);
        auto stepValueItem = new QStandardItem();
        stepValueItem->setData(entity.step, Qt::DisplayRole);
        item->appendRow({stepItem, stepValueItem});
        
        auto valuesItem = new QStandardItem(tr("<Value list>"));
        valuesItem->setEditable(false);
        for (int i = 0; i < entity.values.size(); ++i) {
            auto valueItem = new QStandardItem(QString::number(i));
            valueItem->setEditable(false);
            auto valueValueItem = new QStandardItem();
            valueValueItem->setData(entity.values[i], Qt::DisplayRole);
            valuesItem->appendRow({valueItem, valueValueItem});
        }
        item->appendRow(valuesItem);
        
        return item;
    }

    template <>
    QStandardItem * DspxVisualizationModel::createItem(const QDspx::Master &entity) const {
        auto item = new QStandardItem(tr("<Master>"));
        item->setEditable(false);
        
        item->appendRow(createItem(entity.control));
        
        return item;
    }

    template <>
    QStandardItem * DspxVisualizationModel::createItem(const QDspx::Track &entity) const {
        auto item = new QStandardItem(tr("<Track>"));
        item->setEditable(false);
        
        auto nameItem = new QStandardItem(tr("Name"));
        nameItem->setEditable(false);
        auto nameValueItem = new QStandardItem();
        nameValueItem->setData(entity.name, Qt::DisplayRole);
        item->appendRow({nameItem, nameValueItem});
        
        item->appendRow(createItem(entity.control));
        
        auto clipListItem = new QStandardItem(tr("<Clip list>"));
        clipListItem->setEditable(false);
        for (const auto &clip : entity.clips) {
            clipListItem->appendRow(createItem(*clip));
        }
        item->appendRow(clipListItem);
        
        item->appendRow(createItem(entity.workspace));
        
        return item;
    }

    template <>
    QStandardItem * DspxVisualizationModel::createItem(const QDspx::Workspace &entity) const {
        auto item = new QStandardItem(tr("<Workspace>"));
        item->setEditable(false);
        // Empty implementation as requested
        return item;
    }

    // template <>
    // QStandardItem * DspxVisualizationModel::createItem(const QDspx::Sources &entity) const {
    //     auto item = new QStandardItem(tr("<Sources>"));
    //     item->setEditable(false);
    //     // Empty implementation as requested
    //     return item;
    // }

}
