// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_DSPXPROJECT_CLIPBOARD_H
#define DIFFSCOPE_DSPXPROJECT_CLIPBOARD_H

#include <QObject>

#include <dspxproject/ClipboardData.h>

class QMimeData;

namespace DspxProject {

    class ClipboardPrivate;

    class DSPXPROJECT_EXPORT Clipboard : public QObject {
        Q_OBJECT
        Q_PROPERTY(Mode mode READ mode WRITE setMode NOTIFY changed)
        Q_DECLARE_PRIVATE(Clipboard)

    public:
        enum Mode {
            Global,
            Internal
        };
        Q_ENUM(Mode)

        explicit Clipboard(QObject *parent = nullptr);
        ~Clipboard() override;

        static Clipboard *instance();

        Mode mode() const;
        void setMode(Mode mode);

        void copy(const QList<ClipboardData> &data, QMimeData *additionalMimeData);
        ClipboardData paste(ClipboardData::Type expectedType, bool *ok = nullptr);
        QList<ClipboardData::Type> availablePasteTypes() const;

    Q_SIGNALS:
        void changed();

    private:
        QScopedPointer<ClipboardPrivate> d_ptr;
    };

}

#endif // DIFFSCOPE_DSPXPROJECT_CLIPBOARD_H
