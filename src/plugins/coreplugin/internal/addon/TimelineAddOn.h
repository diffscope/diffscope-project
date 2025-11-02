#ifndef DIFFSCOPE_COREPLUGIN_TIMELINEADDON_H
#define DIFFSCOPE_COREPLUGIN_TIMELINEADDON_H

#include <qqmlintegration.h>

#include <CoreApi/windowinterface.h>

namespace Core::Internal {

    class TimelineAddOn : public WindowInterfaceAddOn {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")

        Q_PROPERTY(QString musicTimeText READ musicTimeText NOTIFY musicTimeTextChanged)
        Q_PROPERTY(QString longTimeText READ longTimeText NOTIFY longTimeTextChanged)
        Q_PROPERTY(bool showMusicTime READ showMusicTime WRITE setShowMusicTime NOTIFY
                       showMusicTimeChanged)
        Q_PROPERTY(bool showAbsoluteTime READ showAbsoluteTime WRITE setShowAbsoluteTime NOTIFY
                       showMusicTimeChanged)
        Q_PROPERTY(int doubleClickInterval READ doubleClickInterval)
    public:
        explicit TimelineAddOn(QObject *parent = nullptr);
        ~TimelineAddOn() override;

        void initialize() override;
        void extensionsInitialized() override;
        bool delayedInitialize() override;

        QString musicTimeText() const;
        QString longTimeText() const;

        bool showMusicTime() const;
        void setShowMusicTime(bool on);

        inline bool showAbsoluteTime() const {
            return !showMusicTime();
        }
        void setShowAbsoluteTime(bool on) {
            setShowMusicTime(!on);
        }

        static int doubleClickInterval();

        Q_INVOKABLE void execQuickJump(const QString &initialText = {}) const;

        Q_INVOKABLE void goToStart() const;
        Q_INVOKABLE void goToPreviousMeasure() const;
        Q_INVOKABLE void goToPreviousBeat() const;
        Q_INVOKABLE void goToPreviousTick() const;
        Q_INVOKABLE void goToEnd() const;
        Q_INVOKABLE void goToNextMeasure() const;
        Q_INVOKABLE void goToNextBeat() const;
        Q_INVOKABLE void goToNextTick() const;

        Q_INVOKABLE void resetProjectTimeRange() const;

    Q_SIGNALS:
        void musicTimeTextChanged();
        void longTimeTextChanged();
        void showMusicTimeChanged();

    private:
        bool m_showMusicTime{true};
    };

}

#endif //DIFFSCOPE_COREPLUGIN_TIMELINEADDON_H
