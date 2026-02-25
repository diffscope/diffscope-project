#ifndef DIFFSCOPE_MIDI_FORMAT_CONVERTER_MIDITEXTCODECCONVERTER_H
#define DIFFSCOPE_MIDI_FORMAT_CONVERTER_MIDITEXTCODECCONVERTER_H

#include <QByteArray>
#include <QList>
#include <QString>

namespace MIDIFormatConverter::Internal {

    class MIDITextCodecConverter {
    public:
        struct CodecInfo {
            QByteArray identifier;
            QString displayName;
        };

        static QList<CodecInfo> availableCodecs();
        static QByteArray detectEncoding(const QByteArray &data);
        static QString decode(const QByteArray &data, const QByteArray &codec);
        static QByteArray encode(const QString &text, const QByteArray &codec);
        static QByteArray defaultCodec();
    };

}

#endif //DIFFSCOPE_MIDI_FORMAT_CONVERTER_MIDITEXTCODECCONVERTER_H
