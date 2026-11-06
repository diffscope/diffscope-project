// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_DSPXPROJECT_DOCUMENT_H
#define DIFFSCOPE_DSPXPROJECT_DOCUMENT_H

#include <optional>

#include <QFlags>
#include <QList>
#include <QObject>
#include <QPair>
#include <QScopedPointer>
#include <qqmlintegration.h>

#include <dspxproject/ClipboardData.h>
#include <dspxproject/dspxprojectglobal.h>

namespace dspx {
    class Document;
    class Model;
    class SelectionModel;
}

namespace opendspx {
    struct Model;
}

namespace Core {
    class TransactionController;
}

namespace DspxProject {

    class DocumentPrivate;

    class DSPXPROJECT_EXPORT Document : public QObject {
        Q_OBJECT
        QML_ELEMENT
        Q_DECLARE_PRIVATE(Document)
        Q_PROPERTY(dspx::Model *model READ model CONSTANT)
        Q_PROPERTY(dspx::SelectionModel *selectionModel READ selectionModel CONSTANT)
        Q_PROPERTY(Core::TransactionController *transactionController READ transactionController CONSTANT)
        Q_PROPERTY(bool anyItemsSelected READ anyItemsSelected NOTIFY anyItemsSelectedChanged)
        Q_PROPERTY(bool editScopeFocused READ isEditScopeFocused NOTIFY editScopeFocusedChanged)
    public:
        enum PasteMode {
            CurrentPosition,
            RelativeToCopiedPlayhead,
            // Clip data retains the original track.
            OriginalPosition,
        };
        Q_ENUM(PasteMode)

        explicit Document(QObject *parent = nullptr);
        ~Document() override;

        dspx::Model *model() const;
        dspx::SelectionModel *selectionModel() const;
        Core::TransactionController *transactionController() const;

        bool anyItemsSelected();
        bool isEditScopeFocused();
        std::optional<ClipboardData::Type> currentClipboardType() const;
        std::optional<ClipboardData> copySelection(int playheadPosition = 0) const;
        int pastePosition(const ClipboardData &data, int playheadPosition, PasteMode mode) const;
        bool pasteRequiresResampling(const ClipboardData &data, int position) const;

        struct PasteResult {
            QList<QObject *> items;
            std::optional<QPair<int, int>> freeParameterRange;
        };

        enum class BounceRisk {
            SingerLayoutMismatch = 0x01,
            DataLoss = 0x02,
            ParameterResampling = 0x04,
        };
        Q_DECLARE_FLAGS(BounceRisks, BounceRisk)
        Q_FLAG(BounceRisks)

        // Model writes require an active transaction.
        std::optional<PasteResult> paste(const ClipboardData &data, int position, PasteMode mode);
        bool deleteSelection();
        QList<QObject *> splitItems(int position);
        bool bounceToClip();
        void loadModel(const opendspx::Model &model);

        Q_INVOKABLE void selectAll();
        Q_INVOKABLE void deselectAll();
        void selectItems(const QList<QObject *> &items, bool clearPrevious = false);
        void selectPasteResult(const PasteResult &result);
        Q_INVOKABLE bool isClipSingerLayoutIdentical() const;
        Q_INVOKABLE bool canSafelyBounce() const;
        std::optional<BounceRisks> bounceRisks() const;

    Q_SIGNALS:
        void anyItemsSelectedChanged();
        void editScopeFocusedChanged();

    private:
        QScopedPointer<DocumentPrivate> d_ptr;
    };

    Q_DECLARE_OPERATORS_FOR_FLAGS(Document::BounceRisks)

}

#endif // DIFFSCOPE_DSPXPROJECT_DOCUMENT_H
