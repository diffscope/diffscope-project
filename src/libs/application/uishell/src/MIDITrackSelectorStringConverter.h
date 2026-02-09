#ifndef DIFFSCOPE_UISHELL_MIDITRACKSELECTORSTRINGCONVERTER_H
#define DIFFSCOPE_UISHELL_MIDITRACKSELECTORSTRINGCONVERTER_H

#include <QByteArray>
#include <QList>
#include <QString>

#include <uishell/UIShellGlobal.h>

namespace UIShell {

    class UISHELL_EXPORT MIDITrackSelectorStringConverter {
    public:
        struct CodecInfo {
            QByteArray identifier;
            QString displayName;
        };

        virtual ~MIDITrackSelectorStringConverter() = default;

        virtual QList<CodecInfo> availableCodecs() const = 0;
        virtual QByteArray detectEncoding(const QByteArray &data) const = 0;
        virtual QString decode(const QByteArray &data, const QByteArray &codec) const = 0;
        virtual QByteArray defaultCodec() const = 0;
    };

}

#endif //DIFFSCOPE_UISHELL_MIDITRACKSELECTORSTRINGCONVERTER_H
