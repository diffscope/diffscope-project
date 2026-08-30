// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#ifndef UISHELL_DIGITALCLOCKLABEL_P_H
#define UISHELL_DIGITALCLOCKLABEL_P_H

#include <qqmlintegration.h>

#include <QFont>
#include <QObject>
#include <QString>

#include <QtQuick/private/qquicktext_p.h>

namespace UIShell {

    class DigitalClockLabel : public QQuickText {
        Q_OBJECT
        QML_ELEMENT

        Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged)
        Q_PROPERTY(QFont font READ font WRITE setFont NOTIFY fontChanged)
        Q_PROPERTY(FineTuneMode fineTuneMode READ fineTuneMode WRITE setFineTuneMode NOTIFY fineTuneModeChanged)

    public:
        enum FineTuneMode {
            None,
            TabularFiguresFontFeature,
            LayoutSimulation,
        };
        Q_ENUM(FineTuneMode)

        explicit DigitalClockLabel(QQuickItem *parent = nullptr);
        ~DigitalClockLabel() override;

        QString text() const;
        void setText(const QString &text);

        QFont font() const;
        void setFont(const QFont &font);

        FineTuneMode fineTuneMode() const;
        void setFineTuneMode(FineTuneMode mode);

    Q_SIGNALS:
        void textChanged();
        void fontChanged();
        void fineTuneModeChanged();

    private:
        void updateDigitalFormat();

        QString m_text;
        QFont m_font;
        FineTuneMode m_fineTuneMode = TabularFiguresFontFeature;

    };

}

#endif // UISHELL_DIGITALCLOCKLABEL_P_H
