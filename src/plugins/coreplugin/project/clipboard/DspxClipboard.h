#ifndef DIFFSCOPE_COREPLUGIN_DSPXCLIPBOARD_H
#define DIFFSCOPE_COREPLUGIN_DSPXCLIPBOARD_H

#include <QObject>

#include <coreplugin/DspxClipboardData.h>

class QMimeData;

namespace Core {

    class DspxClipboardPrivate;

    class CORE_EXPORT DspxClipboard : public QObject {
        Q_OBJECT
        Q_PROPERTY(Mode mode READ mode WRITE setMode NOTIFY changed)
        Q_DECLARE_PRIVATE(DspxClipboard)

    public:
        enum Mode {
            Global,
            Internal
        };
        Q_ENUM(Mode)

        explicit DspxClipboard(QObject *parent = nullptr);
        ~DspxClipboard() override;

        static DspxClipboard *instance();

        Mode mode() const;
        void setMode(Mode mode);

        void copy(const QList<DspxClipboardData> &data, QMimeData *additionalMimeData);
        DspxClipboardData paste(DspxClipboardData::Type expectedType, bool *ok = nullptr);
        QList<DspxClipboardData::Type> availablePasteTypes() const;

    Q_SIGNALS:
        void changed();

    private:
        QScopedPointer<DspxClipboardPrivate> d_ptr;
    };

}

#endif //DIFFSCOPE_COREPLUGIN_DSPXCLIPBOARD_H
