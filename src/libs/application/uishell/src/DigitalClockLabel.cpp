#include "DigitalClockLabel_p.h"

#include <QtQuick/private/qquicktext_p_p.h>

namespace UIShell {

    DigitalClockLabel::DigitalClockLabel(QQuickItem *parent) : QQuickText(parent) {
    }
    DigitalClockLabel::~DigitalClockLabel() = default;

    QString DigitalClockLabel::text() const {
        return m_fullText;
    }

    void DigitalClockLabel::setText(const QString &text) {
        if (m_fullText == text)
            return;

        m_fullText = text;
        updateDigitalFormat();
        Q_EMIT textChanged();
    }

    bool DigitalClockLabel::fineTuneEnabled() const {
        return m_fineTuneEnabled;
    }

    void DigitalClockLabel::setFineTuneEnabled(bool enabled) {
        if (m_fineTuneEnabled == enabled)
            return;

        m_fineTuneEnabled = enabled;
        updateDigitalFormat();

        if (isComponentComplete())
            forceLayout();

        Q_EMIT fineTuneEnabledChanged();
    }

    static inline QTextLayout::FormatRange fixedPitchRange(int start, int length) {
        QTextLayout::FormatRange range;
        range.start = start;
        range.length = length;
        range.format.setFontFixedPitch(true);
        return range;
    }

    static constexpr bool isIncludedChar(QChar c) {
        return c.isDigit();
    }

    void DigitalClockLabel::updateDigitalFormat() {
        // FIXME
        QString text = m_fullText;
        QList<QTextLayout::FormatRange> formats;
        if (m_fineTuneEnabled) {
            formats += fixedPitchRange(0, m_fullText.length());
        }
        QQuickTextPrivate::get(this)->layout.setFormats(formats);
        QQuickText::setText(text);
    }

}

#include "moc_DigitalClockLabel_p.cpp"
