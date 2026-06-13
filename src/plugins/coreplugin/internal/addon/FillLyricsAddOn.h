#ifndef DIFFSCOPE_COREPLUGIN_FILLLYRICSADDON_H
#define DIFFSCOPE_COREPLUGIN_FILLLYRICSADDON_H

#include <qqmlintegration.h>

#include <QString>

#include <CoreApi/windowinterface.h>

namespace Core::Internal {

    class FillLyricsAddOn : public WindowInterfaceAddOn {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
    public:
        explicit FillLyricsAddOn(QObject *parent = nullptr);
        ~FillLyricsAddOn() override;

        void initialize() override;
        void extensionsInitialized() override;
        bool delayedInitialize() override;

        enum SplitMode {
            SplitMode_Auto,
            SplitMode_Character,
            SplitMode_Word,
            SplitMode_Regex,
        };
        Q_ENUM(SplitMode)

        Q_INVOKABLE QString regularExpressionForSplitMode(SplitMode splitMode) const;
        Q_INVOKABLE bool isRegularExpressionValid(const QString &pattern) const;
        Q_INVOKABLE void fillLyrics();
    };

}

#endif // DIFFSCOPE_COREPLUGIN_FILLLYRICSADDON_H
