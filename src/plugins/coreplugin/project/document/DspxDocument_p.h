#ifndef DIFFSCOPE_COREPLUGIN_DSPXDOCUMENT_P_H
#define DIFFSCOPE_COREPLUGIN_DSPXDOCUMENT_P_H

#include <optional>

#include <QList>

#include <coreplugin/DspxDocument.h>
#include <coreplugin/DspxClipboardData.h>

namespace opendspx {
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
        std::optional<DspxClipboardData> buildKeySignatureClipboardData(int playheadPosition) const;
        std::optional<DspxClipboardData> buildTrackClipboardData() const;
        bool copyAnchorNodeSelection(int playheadPosition) const;
        bool copyFreeParameterSelection(int playheadPosition) const;
        bool pasteAnchorNodeSelection(int playheadPosition);
        bool pasteFreeParameterSelection(int playheadPosition);

        bool pasteClipboardData(const DspxClipboardData &data, int playheadPosition, QList<QObject *> &pastedItems);
        bool pasteTempos(const QList<opendspx::Tempo> &tempos, const DspxClipboardData &data, int playheadPosition, QList<QObject *> &pastedItems);
        bool pasteLabels(const QList<opendspx::Label> &labels, const DspxClipboardData &data, int playheadPosition, QList<QObject *> &pastedItems);
        bool pasteKeySignatures(const QList<nlohmann::json> &keySignatures, const DspxClipboardData &data, int playheadPosition, QList<QObject *> &pastedItems);
        bool pasteTracks(const QList<opendspx::Track> &tracks, QList<QObject *> &pastedItems);

        bool deleteSelection();
        int deleteTempos();
        int deleteLabels();
        int deleteKeySignatures();
        int deleteTracks();
        int deleteClips();
        int deleteNotes();
        int deleteAnchorNodes();
        bool deleteFreeParameterSelection();

        void selectAllTempos();
        void selectAllLabels();
        void selectAllKeySignatures();
        void selectAllTracks();
        void selectAllClips();
        void selectAllNotes();
        void selectAllAnchorNodes();
        void selectAllFreeParameter();

        template<typename Signal>
        bool emitOnChange(bool value, bool &cache, Signal signal);

        DspxDocument *q_ptr = nullptr;
        dspx::Document *dspxDocument = nullptr;
        dspx::Model *model = nullptr;
        dspx::SelectionModel *selectionModel = nullptr;
        FreeParameterSelectionModel *freeParameterSelectionModel = nullptr;
        TransactionController *transactionController = nullptr;
        bool anyItemsSelected{false};
        bool editScopeFocused{false};
        bool pasteAvailable{false};
    };
}

#endif //DIFFSCOPE_COREPLUGIN_DSPXDOCUMENT_P_H
