#include "MIDITextCodecConverter.h"

#include <QByteArray>
#include <QList>
#include <QString>
#include <QSet>

#import <Foundation/Foundation.h>
#import <CoreFoundation/CoreFoundation.h>

namespace MIDIFormatConverter::Internal {

    static QByteArray cfStringToUtf8(CFStringRef value) {
        if (value == nullptr) {
            return {};
        }
        const auto length = CFStringGetLength(value);
        const auto maxSize = CFStringGetMaximumSizeForEncoding(length, kCFStringEncodingUTF8) + 1;
        QByteArray buffer;
        buffer.resize(static_cast<int>(maxSize));
        if (!CFStringGetCString(value, buffer.data(), maxSize, kCFStringEncodingUTF8)) {
            return {};
        }
        buffer.truncate(static_cast<int>(strlen(buffer.constData())));
        return buffer;
    }

    static QByteArray encodingIdentifier(NSStringEncoding encoding) {
        const auto cfEncoding = CFStringConvertNSStringEncodingToEncoding(encoding);
        const auto name = CFStringConvertEncodingToIANACharSetName(cfEncoding);
        return cfStringToUtf8(name);
    }

    static NSStringEncoding encodingFromIdentifier(const QByteArray &identifier) {
        if (identifier.isEmpty()) {
            return 0;
        }
        const auto cfName = CFStringCreateWithBytes(kCFAllocatorDefault,
                                                    reinterpret_cast<const UInt8 *>(identifier.constData()),
                                                    identifier.size(),
                                                    kCFStringEncodingUTF8,
                                                    false);
        if (cfName == nullptr) {
            return 0;
        }
        const auto cfEncoding = CFStringConvertIANACharSetNameToEncoding(cfName);
        CFRelease(cfName);
        if (cfEncoding == kCFStringEncodingInvalidId) {
            return 0;
        }
        return CFStringConvertEncodingToNSStringEncoding(cfEncoding);
    }

    QList<MIDITextCodecConverter::CodecInfo> MIDITextCodecConverter::availableCodecs() {
        QList<CodecInfo> result;
        QSet<QByteArray> seen;

        const auto *encodings = CFStringGetListOfAvailableEncodings();
        for (auto encoding = encodings; encoding != nullptr && *encoding != kCFStringEncodingInvalidId; ++encoding) {
            const auto name = CFStringConvertEncodingToIANACharSetName(*encoding);
            if (name == nullptr) {
                continue;
            }
            const QByteArray identifier = cfStringToUtf8(name);
            if (identifier.isEmpty() || seen.contains(identifier)) {
                continue;
            }
            seen.insert(identifier);

            CodecInfo info;
            info.identifier = identifier;
            info.displayName = QString::fromCFString(CFStringGetNameOfEncoding(*encoding));
            if (info.displayName.isEmpty()) {
                info.displayName = QString::fromUtf8(info.identifier);
            }
            result.append(info);
        }

        return result;
    }

    QByteArray MIDITextCodecConverter::detectEncoding(const QByteArray &data) {
        if (data.isEmpty()) {
            return {};
        }

        @autoreleasepool {
            NSData *nsData = [NSData dataWithBytes:data.constData() length:static_cast<NSUInteger>(data.size())];
            NSString *converted = nil;

            const NSStringEncoding encoding = [NSString stringEncodingForData:nsData
                                                               encodingOptions:nil
                                                               convertedString:&converted
                                                         usedLossyConversion:nil];

            if (encoding == 0) {
                return {};
            }

            return encodingIdentifier(encoding);
        }
    }

    QString MIDITextCodecConverter::decode(const QByteArray &data, const QByteArray &codec) {
        if (data.isEmpty()) {
            return {};
        }

        const QByteArray codecName = codec.isEmpty() ? defaultCodec() : codec;
        const NSStringEncoding encoding = encodingFromIdentifier(codecName);
        if (encoding == 0) {
            return {};
        }

        @autoreleasepool {
            NSString *string = [[NSString alloc] initWithBytes:data.constData()
                                                        length:static_cast<NSUInteger>(data.size())
                                                      encoding:encoding];
            if (string == nil) {
                return {};
            }

            NSData *utf8 = [string dataUsingEncoding:NSUTF8StringEncoding allowLossyConversion:NO];
            if (utf8 == nil) {
                return {};
            }

            return QString::fromUtf8(static_cast<const char *>([utf8 bytes]), static_cast<int>([utf8 length]));
        }
    }

    QByteArray MIDITextCodecConverter::encode(const QString &text, const QByteArray &codec) {
        if (text.isEmpty()) {
            return {};
        }

        const QByteArray codecName = codec.isEmpty() ? defaultCodec() : codec;
        const NSStringEncoding encoding = encodingFromIdentifier(codecName);
        if (encoding == 0) {
            return {};
        }

        @autoreleasepool {
            const auto *characters = reinterpret_cast<const unichar *>(text.utf16());
            NSString *string = [[NSString alloc] initWithCharacters:characters length:static_cast<NSUInteger>(text.size())];
            NSData *encoded = [string dataUsingEncoding:encoding allowLossyConversion:NO];
            if (encoded == nil) {
                return {};
            }

            return QByteArray(static_cast<const char *>([encoded bytes]), static_cast<int>([encoded length]));
        }
    }

    QByteArray MIDITextCodecConverter::defaultCodec() {
        return "utf-8";
    }

}
