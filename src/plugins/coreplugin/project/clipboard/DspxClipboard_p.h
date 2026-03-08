#ifndef DIFFSCOPE_COREPLUGIN_DSPXCLIPBOARD_P_H
#define DIFFSCOPE_COREPLUGIN_DSPXCLIPBOARD_P_H

#include <optional>

#include <QHash>
#include <QtGlobal>

#include <coreplugin/DspxClipboard.h>
#include <coreplugin/DspxClipboardData.h>

namespace Core {

    class DspxClipboard;

    class DspxClipboardPrivate {
        Q_DECLARE_PUBLIC(DspxClipboard)
    public:
        void handleClipboardChanged();

        DspxClipboard *q_ptr = nullptr;
        DspxClipboard::Mode mode = DspxClipboard::Global;
        QHash<DspxClipboardData::Type, std::optional<DspxClipboardData>> internalData;
    };

}

#endif //DIFFSCOPE_COREPLUGIN_DSPXCLIPBOARD_P_H