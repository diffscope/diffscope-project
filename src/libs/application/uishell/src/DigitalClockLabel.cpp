// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#include "DigitalClockLabel_p.h"

#include <algorithm>

#include <QFontMetricsF>
#include <QTextLayout>

#include <QtQuick/private/qquicktext_p_p.h>

namespace UIShell {

    DigitalClockLabel::DigitalClockLabel(QQuickItem *parent) : QQuickText(parent) {
    }

    DigitalClockLabel::~DigitalClockLabel() = default;

    QString DigitalClockLabel::text() const {
        return m_text;
    }

    void DigitalClockLabel::setText(const QString &text) {
        if (text == m_text) {
            return;
        }
        m_text = text;
        updateDigitalFormat();
    }

    QFont DigitalClockLabel::font() const {
        return m_font;
    }

    void DigitalClockLabel::setFont(const QFont &font) {
        if (font == m_font) {
            return;
        }
        m_font = font;
        updateDigitalFormat();
        Q_EMIT fontChanged();
    }

    DigitalClockLabel::FineTuneMode DigitalClockLabel::fineTuneMode() const {
        return m_fineTuneMode;
    }

    void DigitalClockLabel::setFineTuneMode(FineTuneMode mode) {
        if (m_fineTuneMode == mode) {
            return;
        }
        m_fineTuneMode = mode;
        updateDigitalFormat();
        Q_EMIT fineTuneModeChanged();
    }

    static QList<QTextLayout::FormatRange> makeTabularDigitFormats(const QString &text, const QFont &font) {
        QFontMetricsF fm(font);

        qreal digitWidth = 0.0;
        for (int i = 0; i <= 9; i++) {
            const QChar ch(static_cast<uchar>('0' + i));
            digitWidth = std::max(digitWidth, fm.horizontalAdvance(ch));
        }

        QList<QTextLayout::FormatRange> formats;
        formats.reserve(text.size());

        for (qsizetype i = 0; i < text.size(); ++i) {
            const QChar ch = text[i];

            if (!ch.isDigit())
                continue;

            const qreal width = fm.horizontalAdvance(ch);
            const qreal extraSpacing = digitWidth - width;

            QTextCharFormat format;

            format.setFontKerning(false);

            format.setFontLetterSpacingType(QFont::AbsoluteSpacing);
            format.setFontLetterSpacing(extraSpacing);

            QTextLayout::FormatRange range;
            range.start = static_cast<int>(i);
            range.length = 1;
            range.format = format;

            formats.append(range);
        }

        return formats;
    }

    void DigitalClockLabel::updateDigitalFormat() {
        auto font = m_font;
        switch (m_fineTuneMode) {
        case None:
            font.unsetFeature("tnum");
            break;
        case TabularFiguresFontFeature:
            font.setFeature("tnum", 1);
            break;
        case LayoutSimulation:
            font.unsetFeature("tnum");
            break;
        }
        QQuickText::setFont(font);
        QQuickText::setText(m_text);
        QList<QTextLayout::FormatRange> formats;
        if (m_fineTuneMode == LayoutSimulation) {
            formats = makeTabularDigitFormats(m_text, font);
        }
        QQuickTextPrivate::get(this)->layout.setFormats(formats);
        if (isComponentComplete()) {
            forceLayout();
        }
    }

}

#include "moc_DigitalClockLabel_p.cpp"
