// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#ifndef DIFFSCOPE_COREPLUGIN_DSPXDOCUMENT_P_H
#define DIFFSCOPE_COREPLUGIN_DSPXDOCUMENT_P_H

#include <coreplugin/DspxDocument.h>

#include <optional>

#include <QList>

#include <coreplugin/DspxClipboardData.h>

namespace dspx {
    class SingingClip;
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
        dspx::SingingClip *clipboardSingingClip() const;
        int currentTrackIndex() const;

        std::optional<DspxClipboardData> buildClipboardData(int playheadPosition) const;
        bool copySelection(int playheadPosition);
        std::optional<DspxClipboardData> buildTempoClipboardData(int playheadPosition) const;
        std::optional<DspxClipboardData> buildLabelClipboardData(int playheadPosition) const;
        std::optional<DspxClipboardData> buildKeySignatureClipboardData(int playheadPosition) const;
        std::optional<DspxClipboardData> buildTrackClipboardData(int playheadPosition) const;
        std::optional<DspxClipboardData> buildClipClipboardData(int playheadPosition) const;
        std::optional<DspxClipboardData> buildNoteClipboardData(int playheadPosition) const;
        std::optional<DspxClipboardData> buildDynamicMixingAnchorClipboardData(int playheadPosition) const;
        std::optional<DspxClipboardData> buildAnchorNodeClipboardData(int playheadPosition) const;
        std::optional<DspxClipboardData> buildFreeParameterClipboardData(int playheadPosition) const;

        struct PasteContext {
            int position{};
            DspxDocument::PasteMode mode{DspxDocument::CurrentPosition};
            QList<QObject *> items;
            int rangeStart{};
            int rangeEnd{};
        };

        int pastePosition(const DspxClipboardData &data, int playheadPosition, DspxDocument::PasteMode mode) const;
        bool pasteTempos(const DspxClipboardData &data, PasteContext &context);
        bool pasteLabels(const DspxClipboardData &data, PasteContext &context);
        bool pasteKeySignatures(const DspxClipboardData &data, PasteContext &context);
        bool pasteTracks(const DspxClipboardData &data, PasteContext &context);
        bool pasteClips(const DspxClipboardData &data, PasteContext &context);
        bool pasteNotes(const DspxClipboardData &data, PasteContext &context);
        bool pasteDynamicMixingAnchors(const DspxClipboardData &data, PasteContext &context);
        bool pasteAnchorNodes(const DspxClipboardData &data, PasteContext &context);
        bool pasteFreeParameter(const DspxClipboardData &data, PasteContext &context);

        bool canSafelyBounceIgnoringResampling() const;
        bool bounceRequiresResampling() const;

        bool deleteSelection();
        int deleteTempos();
        int deleteLabels();
        int deleteKeySignatures();
        int deleteTracks();
        int deleteClips();
        int deleteNotes();
        int deleteAnchorNodes();
        int deleteDynamicMixingAnchors();
        bool deleteFreeParameterSelection();

        void selectAllTempos();
        void selectAllLabels();
        void selectAllKeySignatures();
        void selectAllTracks();
        void selectAllClips();
        void selectAllNotes();
        void selectAllAnchorNodes();
        void selectAllDynamicMixingAnchors();
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
