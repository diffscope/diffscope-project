// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_DSPXPROJECT_DOCUMENT_H
#define DIFFSCOPE_DSPXPROJECT_DOCUMENT_H

#include <optional>

#include <QFlags>
#include <QList>
#include <QObject>
#include <QPair>
#include <QPointer>
#include <QScopedPointer>
#include <QString>
#include <qqmlintegration.h>

#include <dspxmodelORM/FreeValueDataArray.h>

#include <dspxproject/ClipboardData.h>
#include <dspxproject/dspxprojectglobal.h>

namespace dspx {
    class Document;
    class Model;
    class SelectionModel;
    class SingingClip;
    class Sources;
    class Track;
}

namespace opendspx {
    struct Model;
}

namespace Core {
    class TransactionController;
}

namespace DspxProject {

    class DocumentProblemManager;
    class DocumentPrivate;

    /**
     * @brief Owns the project model, selection, and transaction controller.
     *
     * Model edits require an active transaction. Callers must abort the transaction
     * if an operation fails. Selection changes do not require a transaction.
     * Returned model objects are borrowed and remain valid until removed or until
     * the document is destroyed.
     */
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
        /** The placement policy for pasted data. */
        enum PasteMode {
            CurrentPosition, /**< Places the payload origin at the current playhead. */
            RelativeToCopiedPlayhead, /**< Preserves the offset from the copied playhead. */
            OriginalPosition, /**< Restores the original position and source track for clips. */
        };
        Q_ENUM(PasteMode)

        /** Creates an empty model with a selection model and transaction controller. */
        explicit Document(QObject *parent = nullptr);
        /** Destroys the document and its owned services. */
        ~Document() override;

        /** Returns the non-null owned model. */
        dspx::Model *model() const;
        /** Returns the non-null owned selection model. */
        dspx::SelectionModel *selectionModel() const;
        /** Returns the non-null owned transaction controller. */
        Core::TransactionController *transactionController() const;
        /** Returns the non-null owned problem manager. */
        DocumentProblemManager *problemManager() const;

        /** Returns whether the current edit scope contains a selection. */
        bool anyItemsSelected();
        /** Returns whether an edit scope other than ST_None is active. */
        bool isEditScopeFocused();
        /**
         * Copies the selection with the playhead in project ticks.
         * Returns nullopt if no selection can be copied. Does not modify the clipboard.
         */
        std::optional<ClipboardData> copySelection(int playheadPosition = 0) const;

        /** A resolved destination for one clipboard type and placement mode. */
        class PasteTarget {
        public:
            /** Returns the clipboard type. */
            ClipboardData::Type type() const {
                return m_type;
            }

        private:
            friend class Document;
            friend class DocumentPrivate;

            PasteTarget(ClipboardData::Type type, PasteMode mode) : m_type(type), m_mode(mode) {
            }
            bool operator==(const PasteTarget &other) const;

            ClipboardData::Type m_type;
            PasteMode m_mode;
            QPointer<dspx::SingingClip> m_clip;
            QPointer<dspx::Sources> m_sources;
            std::optional<QPointer<dspx::Track>> m_track;
            QString m_parameterId;
            dspx::FreeValueDataArray::FreeValueRole m_parameterRole = dspx::FreeValueDataArray::Edited;
        };

        /**
         * Returns the destination for the active selection type if available.
         * Otherwise returns the sole valid destination, or nullopt if absent or ambiguous.
         */
        std::optional<PasteTarget> resolvePasteTarget(const QList<ClipboardData::Type> &types, PasteMode mode = CurrentPosition) const;
        /** Returns whether the current selection still resolves to target for its type and mode. */
        bool isPasteTargetValid(const PasteTarget &target) const;
        /**
         * Returns the paste origin for target and a project playhead position.
         * The result is clamped to [0, INT_MAX] and uses clip content ticks for clip
         * contents or project ticks otherwise.
         * @pre target was resolved for this document and data.type() == target.type().
         */
        int pastePosition(const ClipboardData &data, int playheadPosition, const PasteTarget &target) const;
        /**
         * Returns whether a free parameter paste requires interpolation at position.
         * Returns false for other types. position uses the pastePosition() coordinate system.
         */
        bool pasteRequiresResampling(const ClipboardData &data, int position) const;

        /** The inserted objects or replaced free parameter interval from a successful paste. */
        struct PasteResult {
            /** The borrowed inserted objects, empty for a free parameter paste. */
            QList<QObject *> items;
            /** The half-open interval in clip content ticks, absent for object payloads. */
            std::optional<QPair<int, int>> freeParameterRange;
        };

        /** The risks associated with bouncing selected singing clips. */
        enum class BounceRisk {
            SingerLayoutMismatch = 0x01, /**< Singer definitions differ within a track. */
            DataLoss = 0x02, /**< Cropping or overlapping data can discard information. */
            ParameterResampling = 0x04, /**< Free parameter grids require interpolation. */
        };
        /** The combined bounce risks. */
        Q_DECLARE_FLAGS(BounceRisks, BounceRisk)
        Q_FLAG(BounceRisks)

        /**
         * Pastes data into target at the specified paste origin.
         * Returns the inserted objects or replaced interval on success, or nullopt
         * on failure or no change. Clip groups create destination tracks as needed.
         * @pre A transaction is active and position is nonnegative.
         * @pre data satisfies the ClipboardData payload contract.
         */
        std::optional<PasteResult> paste(const ClipboardData &data, int position, const PasteTarget &target);
        /**
         * Deletes the selection, preserving the sole tempo at tick zero.
         * Returns whether data changed. @pre A transaction is active.
         */
        bool deleteSelection();
        /**
         * Splits selected clips or notes at position and returns the inserted right parts.
         * Returns an empty list if no item can be split. position uses project ticks
         * for clips and clip content ticks for notes. @pre A transaction is active.
         */
        QList<QObject *> splitItems(int position);
        /**
         * Bounces selected singing clips into a single singing clip per track.
         * Returns false without selected singing clips, or true after processing.
         * Call bounceRisks() before editing to obtain risks requiring confirmation.
         * @pre A transaction is active.
         */
        bool bounceToClip();
        /** Loads serialized model data. @pre A model storage transaction is active. */
        void loadModel(const opendspx::Model &model);

        /** Selects all items in the active edit scope, or the visible free parameter interval. */
        Q_INVOKABLE void selectAll();
        /** Clears the selection while retaining the active edit scope. */
        Q_INVOKABLE void deselectAll();
        /**
         * Selects items in order and makes each item current.
         * Clears the previous selection before the first item if clearPrevious is true.
         * An empty list leaves selection unchanged.
         * @pre Each item is non-null and is a selectable object in this document.
         */
        void selectItems(const QList<QObject *> &items, bool clearPrevious = false);
        /** Selects a successful paste result from target. @pre result refers to this document's current data. */
        void selectPasteResult(const PasteResult &result, const PasteTarget &target);
        /** Returns whether selected singing clips have identical singer layouts within each track. */
        Q_INVOKABLE bool isClipSingerLayoutIdentical() const;
        /** Returns whether singing clips are selected and bouncing avoids data loss and resampling. */
        Q_INVOKABLE bool canSafelyBounce() const;
        /** Returns the bounce risks, or nullopt if no singing clip selection is active. */
        std::optional<BounceRisks> bounceRisks() const;

    Q_SIGNALS:
        /** Notifies that selection availability changed. */
        void anyItemsSelectedChanged();
        /** Notifies that edit scope focus changed. */
        void editScopeFocusedChanged();
        /** Notifies that selection changes may affect paste targets. */
        void pasteTargetsChanged();

    private:
        QScopedPointer<DocumentPrivate> d_ptr;
    };

    Q_DECLARE_OPERATORS_FOR_FLAGS(Document::BounceRisks)

}

#endif // DIFFSCOPE_DSPXPROJECT_DOCUMENT_H
