// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: GPL-3.0-only

#ifndef DIFFSCOPE_LIBRESVIP_FORMAT_CONVERTER_PREBUILTINFORMATS_H
#define DIFFSCOPE_LIBRESVIP_FORMAT_CONVERTER_PREBUILTINFORMATS_H

#include <QList>
#include <QString>
#include <QStringList>

namespace LibreSVIPFormatConverter::Internal {

    struct PreBuiltInFormat {
        QString identifier;
        QString name;
        QStringList suffixes;
    };

    inline QList<PreBuiltInFormat> preBuiltInInputFormats() {
        return {
            {QStringLiteral("acep"), QStringLiteral("ACE Studio"), {QStringLiteral("acep"), QStringLiteral("acet")}},
            {QStringLiteral("aisp"), QStringLiteral("AISingers"), {QStringLiteral("aisp")}},
            {QStringLiteral("ccs"), QStringLiteral("CeVIO"), {QStringLiteral("ccs")}},
            {QStringLiteral("ds"), QStringLiteral("DiffSinger"), {QStringLiteral("ds")}},
            {QStringLiteral("dspx"), QStringLiteral("DSPX"), {QStringLiteral("dspx")}},
            {QStringLiteral("dv"), QStringLiteral("DeepVocal"), {QStringLiteral("dv"), QStringLiteral("sk")}},
            {QStringLiteral("json"), QStringLiteral("JsonSvip"), {QStringLiteral("json")}},
            {QStringLiteral("mid"), QStringLiteral("MIDI"), {QStringLiteral("mid"), QStringLiteral("midi")}},
            {QStringLiteral("mtp"), QStringLiteral("Muta"), {QStringLiteral("mtp")}},
            {QStringLiteral("musicxml"), QStringLiteral("MusicXML"), {QStringLiteral("musicxml"), QStringLiteral("xml"), QStringLiteral("mxl")}},
            {QStringLiteral("nn"), QStringLiteral("NIAONiao"), {QStringLiteral("nn")}},
            {QStringLiteral("ppsf"), QStringLiteral("Ppsf"), {QStringLiteral("ppsf")}},
            {QStringLiteral("ps_project"), QStringLiteral("PocketSinger"), {QStringLiteral("ps_project")}},
            {QStringLiteral("s5p"), QStringLiteral("SynthV Editor"), {QStringLiteral("s5p")}},
            {QStringLiteral("svip"), QStringLiteral("BinSvip"), {QStringLiteral("svip")}},
            {QStringLiteral("svip3"), QStringLiteral("Svip3"), {QStringLiteral("svip3")}},
            {QStringLiteral("svp"), QStringLiteral("SynthV Studio"), {QStringLiteral("svp")}},
            {QStringLiteral("tlp"), QStringLiteral("TuneLab Legacy"), {QStringLiteral("tlp")}},
            {QStringLiteral("tlpx"), QStringLiteral("TuneLab"), {QStringLiteral("tlpx")}},
            {QStringLiteral("tsmsln"), QStringLiteral("TSMSln"), {QStringLiteral("tsmsln")}},
            {QStringLiteral("tssln"), QStringLiteral("TSSln"), {QStringLiteral("tssln")}},
            {QStringLiteral("ufdata"), QStringLiteral("UFData"), {QStringLiteral("ufdata")}},
            {QStringLiteral("ust"), QStringLiteral("Ust"), {QStringLiteral("ust")}},
            {QStringLiteral("ustx"), QStringLiteral("Ustx"), {QStringLiteral("ustx")}},
            {QStringLiteral("vfp"), QStringLiteral("VOX Factory"), {QStringLiteral("vfp")}},
            {QStringLiteral("vog"), QStringLiteral("Vogen"), {QStringLiteral("vog")}},
            {QStringLiteral("vpr"), QStringLiteral("Vpr"), {QStringLiteral("vpr")}},
            {QStringLiteral("vshp"), QStringLiteral("VocalShifter"), {QStringLiteral("vshp")}},
            {QStringLiteral("vspx"), QStringLiteral("VocalSharp"), {QStringLiteral("vspx")}},
            {QStringLiteral("vsq"), QStringLiteral("Vsq"), {QStringLiteral("vsq")}},
            {QStringLiteral("vsqx"), QStringLiteral("Vsqx"), {QStringLiteral("vsqx")}},
            {QStringLiteral("vvproj"), QStringLiteral("VVProj"), {QStringLiteral("vvproj")}},
            {QStringLiteral("vxf"), QStringLiteral("VX-β"), {QStringLiteral("vxf")}},
            {QStringLiteral("xvsq"), QStringLiteral("Cadencii"), {QStringLiteral("xvsq")}},
            {QStringLiteral("y77"), QStringLiteral("Y77"), {QStringLiteral("y77")}},
        };
    }

    inline QList<PreBuiltInFormat> preBuiltInOutputFormats() {
        return {
            {QStringLiteral("acep"), QStringLiteral("ACE Studio"), {QStringLiteral("acep")}},
            {QStringLiteral("aisp"), QStringLiteral("AISingers"), {QStringLiteral("aisp")}},
            {QStringLiteral("ass"), QStringLiteral("ASS"), {QStringLiteral("ass")}},
            {QStringLiteral("ccs"), QStringLiteral("CeVIO"), {QStringLiteral("ccs")}},
            {QStringLiteral("ds"), QStringLiteral("DiffSinger"), {QStringLiteral("ds")}},
            {QStringLiteral("dspx"), QStringLiteral("DSPX"), {QStringLiteral("dspx")}},
            {QStringLiteral("dv"), QStringLiteral("DeepVocal"), {QStringLiteral("dv")}},
            {QStringLiteral("json"), QStringLiteral("JsonSvip"), {QStringLiteral("json")}},
            {QStringLiteral("lrc"), QStringLiteral("LRC"), {QStringLiteral("lrc")}},
            {QStringLiteral("mid"), QStringLiteral("MIDI"), {QStringLiteral("mid")}},
            {QStringLiteral("mtp"), QStringLiteral("Muta"), {QStringLiteral("mtp")}},
            {QStringLiteral("musicxml"), QStringLiteral("MusicXML"), {QStringLiteral("musicxml")}},
            {QStringLiteral("nn"), QStringLiteral("NIAONiao"), {QStringLiteral("nn")}},
            {QStringLiteral("ppsf"), QStringLiteral("Ppsf"), {QStringLiteral("ppsf")}},
            {QStringLiteral("ps_project"), QStringLiteral("PocketSinger"), {QStringLiteral("ps_project")}},
            {QStringLiteral("s5p"), QStringLiteral("SynthV Editor"), {QStringLiteral("s5p")}},
            {QStringLiteral("srt"), QStringLiteral("SRT"), {QStringLiteral("srt")}},
            {QStringLiteral("svg"), QStringLiteral("Svg"), {QStringLiteral("svg")}},
            {QStringLiteral("svip"), QStringLiteral("BinSvip"), {QStringLiteral("svip")}},
            {QStringLiteral("svip3"), QStringLiteral("Svip3"), {QStringLiteral("svip3")}},
            {QStringLiteral("svp"), QStringLiteral("SynthV Studio"), {QStringLiteral("svp")}},
            {QStringLiteral("tlp"), QStringLiteral("TuneLab Legacy"), {QStringLiteral("tlp")}},
            {QStringLiteral("tlpx"), QStringLiteral("TuneLab"), {QStringLiteral("tlpx")}},
            {QStringLiteral("tsmsln"), QStringLiteral("TSMSln"), {QStringLiteral("tsmsln")}},
            {QStringLiteral("tssln"), QStringLiteral("TSSln"), {QStringLiteral("tssln")}},
            {QStringLiteral("ufdata"), QStringLiteral("UFData"), {QStringLiteral("ufdata")}},
            {QStringLiteral("ust"), QStringLiteral("Ust"), {QStringLiteral("ust")}},
            {QStringLiteral("ustx"), QStringLiteral("Ustx"), {QStringLiteral("ustx")}},
            {QStringLiteral("vfp"), QStringLiteral("VOX Factory"), {QStringLiteral("vfp")}},
            {QStringLiteral("vog"), QStringLiteral("Vogen"), {QStringLiteral("vog")}},
            {QStringLiteral("vpr"), QStringLiteral("Vpr"), {QStringLiteral("vpr")}},
            {QStringLiteral("vspx"), QStringLiteral("VocalSharp"), {QStringLiteral("vspx")}},
            {QStringLiteral("vsq"), QStringLiteral("Vsq"), {QStringLiteral("vsq")}},
            {QStringLiteral("vsqx"), QStringLiteral("Vsqx"), {QStringLiteral("vsqx")}},
            {QStringLiteral("vvproj"), QStringLiteral("VVProj"), {QStringLiteral("vvproj")}},
            {QStringLiteral("vxf"), QStringLiteral("VX-β"), {QStringLiteral("vxf")}},
            {QStringLiteral("xvsq"), QStringLiteral("Cadencii"), {QStringLiteral("xvsq")}},
            {QStringLiteral("y77"), QStringLiteral("Y77"), {QStringLiteral("y77")}},
        };
    }

    inline QStringList preBuiltInFormatHeuristicFilters(const QList<PreBuiltInFormat> &formats) {
        QStringList filters;
        for (const auto &format : formats) {
            for (const auto &suffix : format.suffixes)
                filters.append(QStringLiteral("*.%1").arg(suffix));
        }
        filters.removeDuplicates();
        return filters;
    }

    inline QStringList preBuiltInFormatFileDialogFilters(const QList<PreBuiltInFormat> &formats) {
        QStringList filters;
        filters.append(QStringLiteral("All Supported Formats (%1)")
                           .arg(preBuiltInFormatHeuristicFilters(formats).join(QLatin1Char(' '))));
        for (const auto &format : formats) {
            QStringList patterns;
            for (const auto &suffix : format.suffixes)
                patterns.append(QStringLiteral("*.%1").arg(suffix));
            filters.append(QStringLiteral("%1 (%2)").arg(format.name, patterns.join(QLatin1Char(' '))));
        }
        return filters;
    }

}

#endif // DIFFSCOPE_LIBRESVIP_FORMAT_CONVERTER_PREBUILTINFORMATS_H
