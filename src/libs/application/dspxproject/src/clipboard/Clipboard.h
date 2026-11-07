// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_DSPXPROJECT_CLIPBOARD_H
#define DIFFSCOPE_DSPXPROJECT_CLIPBOARD_H

#include <QObject>

#include <dspxproject/ClipboardData.h>

class QMimeData;

namespace DspxProject {

    class ClipboardPrivate;

    /** Provides application clipboard access through the system or internal storage. */
    class DSPXPROJECT_EXPORT Clipboard : public QObject {
        Q_OBJECT
        Q_PROPERTY(Mode mode READ mode WRITE setMode NOTIFY changed)
        Q_DECLARE_PRIVATE(Clipboard)

    public:
        /** The storage used by subsequent clipboard operations. */
        enum Mode {
            Global, /**< The system clipboard. */
            Internal /**< Application-local storage independent of the system clipboard. */
        };
        Q_ENUM(Mode)

        /** Creates the clipboard in Global mode. @pre No Clipboard instance exists. */
        explicit Clipboard(QObject *parent = nullptr);
        /** Destroys the clipboard and clears instance(). */
        ~Clipboard() override;

        /** Returns the current instance, or nullptr if no instance exists. */
        static Clipboard *instance();

        /** Returns the active storage mode. */
        Mode mode() const;
        /** Sets the storage mode without transferring data between stores. */
        void setMode(Mode mode);

        /**
         * Replaces the active clipboard contents with data.
         * Only the first payload of each type is retained. In Global mode, a non-null
         * additionalMimeData supplies extra formats and takes precedence for formats
         * already present. Ownership transfers to the system clipboard. A null pointer
         * creates new MIME data. In Internal mode, additionalMimeData is ignored and
         * ownership remains with the caller. An empty data list clears internal data
         * or publishes only additionalMimeData in Global mode.
         */
        void copy(const QList<ClipboardData> &data, QMimeData *additionalMimeData);
        /**
         * Returns the requested payload, or default data if unavailable or invalid.
         * Writes success to ok if non-null. The result is usable only on success.
         */
        ClipboardData paste(ClipboardData::Type expectedType, bool *ok = nullptr);
        /** Returns the advertised payload types without validating their encoded data. */
        QList<ClipboardData::Type> availablePasteTypes() const;

    Q_SIGNALS:
        /** Notifies that the active clipboard contents or storage mode changed. */
        void changed();

    private:
        QScopedPointer<ClipboardPrivate> d_ptr;
    };

}

#endif // DIFFSCOPE_DSPXPROJECT_CLIPBOARD_H
