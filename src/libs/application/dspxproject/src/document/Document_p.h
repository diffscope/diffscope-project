// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_DSPXPROJECT_DOCUMENT_P_H
#define DIFFSCOPE_DSPXPROJECT_DOCUMENT_P_H

#include <dspxproject/Document.h>

#include <optional>

#include <QList>

#include <dspxproject/ClipboardData.h>

namespace dspx {
    class SingingClip;
}

namespace DspxProject {
    class DocumentPrivate {
        Q_DECLARE_PUBLIC(Document)
    public:

        void initConnections();

        bool updateEditScopeFocused();

        std::optional<ClipboardData::Type> currentClipboardType() const;
        dspx::SingingClip *clipboardSingingClip() const;
        int currentTrackIndex() const;

        std::optional<ClipboardData> buildClipboardData(int playheadPosition) const;
        std::optional<ClipboardData> buildTempoClipboardData(int playheadPosition) const;
        std::optional<ClipboardData> buildLabelClipboardData(int playheadPosition) const;
        std::optional<ClipboardData> buildKeySignatureClipboardData(int playheadPosition) const;
        std::optional<ClipboardData> buildTrackClipboardData(int playheadPosition) const;
        std::optional<ClipboardData> buildClipClipboardData(int playheadPosition) const;
        std::optional<ClipboardData> buildNoteClipboardData(int playheadPosition) const;
        std::optional<ClipboardData> buildDynamicMixingAnchorClipboardData(int playheadPosition) const;
        std::optional<ClipboardData> buildAnchorNodeClipboardData(int playheadPosition) const;
        std::optional<ClipboardData> buildFreeParameterClipboardData(int playheadPosition) const;

        struct PasteContext {
            int position{};
            Document::PasteMode mode{Document::CurrentPosition};
            QList<QObject *> items;
            int rangeStart{};
            int rangeEnd{};
        };

        int pastePosition(const ClipboardData &data, int playheadPosition, Document::PasteMode mode) const;
        bool pasteTempos(const ClipboardData &data, PasteContext &context);
        bool pasteLabels(const ClipboardData &data, PasteContext &context);
        bool pasteKeySignatures(const ClipboardData &data, PasteContext &context);
        bool pasteTracks(const ClipboardData &data, PasteContext &context);
        bool pasteClips(const ClipboardData &data, PasteContext &context);
        bool pasteNotes(const ClipboardData &data, PasteContext &context);
        bool pasteDynamicMixingAnchors(const ClipboardData &data, PasteContext &context);
        bool pasteAnchorNodes(const ClipboardData &data, PasteContext &context);
        bool pasteFreeParameter(const ClipboardData &data, PasteContext &context);

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

        Document *q_ptr = nullptr;
        dspx::Document *dspxDocument = nullptr;
        dspx::Model *model = nullptr;
        dspx::SelectionModel *selectionModel = nullptr;
        Core::TransactionController *transactionController = nullptr;
        bool editScopeFocused{false};
    };
}

#endif // DIFFSCOPE_DSPXPROJECT_DOCUMENT_P_H
