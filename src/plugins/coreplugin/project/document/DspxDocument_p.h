#ifndef DIFFSCOPE_COREPLUGIN_DSPXDOCUMENT_P_H
#define DIFFSCOPE_COREPLUGIN_DSPXDOCUMENT_P_H

#include <optional>

#include <QList>

#include <coreplugin/DspxDocument.h>
#include <coreplugin/DspxClipboardData.h>

namespace QDspx {
    struct Tempo;
    struct Label;
    struct Track;
}

namespace Core {
    class DspxDocumentPrivate {
        Q_DECLARE_PUBLIC(DspxDocument)
    public:
        void initConnections();

        bool updateAnyItemsSelected();
        bool updateEditScopeFocused();
        bool updatePasteAvailable();

        std::optional<DspxClipboardData::Type> currentClipboardType() const;

        std::optional<DspxClipboardData> buildClipboardData(int playheadPosition) const;
        std::optional<DspxClipboardData> buildTempoClipboardData(int playheadPosition) const;
        std::optional<DspxClipboardData> buildLabelClipboardData(int playheadPosition) const;
        std::optional<DspxClipboardData> buildTrackClipboardData() const;

        bool pasteClipboardData(const DspxClipboardData &data, int playheadPosition, QList<QObject *> &pastedItems);
        bool pasteTempos(const QList<QDspx::Tempo> &tempos, const DspxClipboardData &data, int playheadPosition, QList<QObject *> &pastedItems);
        bool pasteLabels(const QList<QDspx::Label> &labels, const DspxClipboardData &data, int playheadPosition, QList<QObject *> &pastedItems);
        bool pasteTracks(const QList<QDspx::Track> &tracks, QList<QObject *> &pastedItems);

        bool deleteSelection();
        int deleteTempos();
        int deleteLabels();
        int deleteTracks();

        template<typename Signal>
        bool emitOnChange(bool value, bool &cache, Signal signal);

        DspxDocument *q_ptr = nullptr;
        dspx::Model *model = nullptr;
        dspx::SelectionModel *selectionModel = nullptr;
        TransactionController *transactionController = nullptr;
        bool anyItemsSelected{false};
        bool editScopeFocused{false};
        bool pasteAvailable{false};
    };
}

#endif //DIFFSCOPE_COREPLUGIN_DSPXDOCUMENT_P_H
