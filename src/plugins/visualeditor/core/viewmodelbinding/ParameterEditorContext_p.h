#ifndef DIFFSCOPE_VISUALEDITOR_PARAMETEREDITORCONTEXT_P_H
#define DIFFSCOPE_VISUALEDITOR_PARAMETEREDITORCONTEXT_P_H

#include <array>
#include <limits>

#include <QAbstractListModel>
#include <QHash>
#include <QMetaObject>
#include <QPointer>
#include <QSet>
#include <QVector>

#include <ScopicFlowCore/ParameterAnchorViewModel.h>

#include <dspxmodelORM/Parameter.h>

#include <transactional/TransactionController.h>

#include <visualeditor/ParameterEditorContext.h>

class QState;
class QStateMachine;

namespace dspx {
    class AnchorNode;
    class AnchorNodeSequence;
    class FreeValueDataArray;
    class Model;
    class ParameterMap;
    class SelectionModel;
    class Sources;
}

namespace Core {
    class DspxDocument;
    class FreeParameterSelectionModel;
    class SingerRegistry;
}

namespace sflow {
    class ParameterAnchorViewModel;
}

namespace VisualEditor {

    class AnchorParameterSelectionController;

    class ParameterDefinitionListModel : public QAbstractListModel {
        Q_OBJECT

    public:
        enum Role {
            ParameterIdRole = Qt::UserRole + 1,
            DisplayNameRole,
            RegisteredRole,
            WarningRole,
            NoneRole,
            ParameterInfoRole,
        };

        struct Entry {
            QString parameterId;
            QString displayName;
            Core::ParameterInfo parameterInfo;
            bool registered{};
            bool none{};
        };

        explicit ParameterDefinitionListModel(QObject *parent = nullptr);

        int rowCount(const QModelIndex &parent = {}) const override;
        QVariant data(const QModelIndex &index, int role) const override;
        QHash<int, QByteArray> roleNames() const override;

        void setEntries(QList<Entry> entries);
        const Entry *find(const QString &parameterId) const;
        const Entry *firstAvailable() const;

    private:
        QList<Entry> m_entries;
    };

    class ParameterViewModelBindingPrivate : public QObject {
        Q_OBJECT
        Q_DECLARE_PUBLIC(ParameterViewModelBinding)

    public:
        enum Operation {
            NoOperation,
            FreeEditing,
            AnchorInsertion,
            AnchorMoving,
            AnchorDeletion,
            AnchorConversion,
        };

        struct OperationStates {
            QState *pending{};
            QState *progressing{};
            QState *committing{};
            QState *aborting{};
        };

        explicit ParameterViewModelBindingPrivate(ParameterViewModelBinding *q, bool editable);

        ParameterViewModelBinding *q_ptr{};
        bool editable{};
        Core::DspxDocument *document{};
        dspx::Model *model{};
        dspx::SelectionModel *selectionModel{};
        Core::FreeParameterSelectionModel *freeSelectionModel{};
        QPointer<dspx::SingingClip> singingClip;
        QPointer<dspx::Parameter> parameter;
        QString parameterId;
        Core::ParameterInfo parameterInfo;
        bool registered{};

        sflow::FreeParameterViewModel *original{};
        sflow::FreeParameterViewModel *freeEdited{};
        sflow::FreeParameterViewModel *freeTransform{};
        sflow::AnchorParameterViewModel *anchorEdited{};
        sflow::AnchorParameterViewModel *anchorTransform{};
        sflow::ParameterRangeSelectionViewModel *freeSelection{};
        sflow::ParameterRangeSelectionViewModel *transformFreeSelection{};
        AnchorParameterSelectionController *anchorSelectionController{};
        AnchorParameterSelectionController *transformAnchorSelectionController{};
        sflow::ParameterEditorInteractionController *interactionController{};
        sflow::ParameterEditorInteractionController *transformInteractionController{};

        QHash<dspx::AnchorNode *, sflow::ParameterAnchorViewModel *> anchorViewItems;
        QHash<sflow::ParameterAnchorViewModel *, dspx::AnchorNode *> anchorDocumentItems;
        QHash<dspx::AnchorNode *, sflow::ParameterAnchorViewModel *> transformAnchorViewItems;
        QHash<sflow::ParameterAnchorViewModel *, dspx::AnchorNode *> transformAnchorDocumentItems;
        QSet<sflow::ParameterAnchorViewModel *> dirtyAnchors;
        QSet<sflow::ParameterAnchorViewModel *> insertedAnchors;
        QSet<sflow::ParameterAnchorViewModel *> removedAnchorViews;
        QSet<dspx::AnchorNode *> removedAnchors;
        QSet<sflow::ParameterAnchorViewModel *> pendingSelectedAnchors;
        QPointer<sflow::ParameterAnchorViewModel> pendingCurrentAnchor;
        int freeDirtyFirst{std::numeric_limits<int>::max()};
        int freeDirtyLast{-1};
        bool updatingView{};
        bool updatingDocument{};
        bool updatingSelection{};
        bool creatingParameter{};

        QVector<QMetaObject::Connection> parameterConnections;
        QVector<QMetaObject::Connection> targetConnections;

        QStateMachine *stateMachine{};
        QState *idleState{};
        std::array<OperationStates, 5> operationStates{};
        Operation currentOperation{NoOperation};
        bool currentOperationTransforms{};
        Core::TransactionController::TransactionId transactionId{};

        void initialize(Core::DspxDocument *document);
        void initializeStateMachine();
        void initializeController();
        void setTarget(dspx::SingingClip *clip, const QString &id,
                       const Core::ParameterInfo &info, bool isRegistered);
        void abortActiveEdit();
        void updateControllerDefinition();

        void bindParameter(dspx::Parameter *newParameter);
        void unbindParameter();
        void reloadFromDocument();
        void clearViewModels();
        void bindFreeArray(dspx::FreeValueDataArray *array, sflow::FreeParameterViewModel *viewModel,
                           bool transform = false);
        void bindAnchorSequence(dspx::AnchorNodeSequence *sequence,
                                sflow::AnchorParameterViewModel *viewModel, bool edited);
        void bindAnchor(dspx::AnchorNode *item, sflow::AnchorParameterViewModel *viewModel, bool edited,
                        sflow::ParameterAnchorViewModel *existingViewItem = nullptr);
        void unbindAnchor(dspx::AnchorNode *item, sflow::AnchorParameterViewModel *viewModel, bool edited);

        Core::ParameterInfo conversionInfo(bool transform) const;
        QList<QVariant> normalizeValues(const QList<QVariant> &values, bool transform = false) const;
        QList<QVariant> denormalizeValues(const QList<QVariant> &values, bool transform = false) const;
        int canonicalRawValue(double normalizedValue, bool transform = false) const;
        double canonicalNormalizedValue(double normalizedValue, bool transform = false) const;
        static sflow::ParameterAnchorViewModel::InterpolationMode toViewInterpolation(int mode);
        static int toDocumentInterpolation(sflow::ParameterAnchorViewModel::InterpolationMode mode);

        void startOperation(Operation operation, bool transform = false);
        void commitOperation(Operation operation, bool transform = false);
        void abortOperation(Operation operation, bool transform = false);
        void enterPending(Operation operation);
        void enterCommitting(Operation operation);
        void enterAborting(Operation operation);
        bool commitChanges();
        dspx::Parameter *createParameter();
        bool commitFreeChanges();
        bool commitAnchorChanges();
        bool hasChanges() const;
        void resetDirtyState();

        void emitWillStart(Operation operation);
        void emitStarted(Operation operation);
        void emitNotStarted(Operation operation);
        void emitWillCommit(Operation operation);
        void emitWillAbort(Operation operation);

    Q_SIGNALS:
        void freeWillStart();
        void freeStarted();
        void freeNotStarted();
        void freeWillCommit();
        void freeWillAbort();
        void insertionWillStart();
        void insertionStarted();
        void insertionNotStarted();
        void insertionWillCommit();
        void insertionWillAbort();
        void movingWillStart();
        void movingStarted();
        void movingNotStarted();
        void movingWillCommit();
        void movingWillAbort();
        void deletionWillStart();
        void deletionStarted();
        void deletionNotStarted();
        void deletionWillCommit();
        void deletionWillAbort();
        void conversionWillStart();
        void conversionStarted();
        void conversionNotStarted();
        void conversionWillCommit();
        void conversionWillAbort();
    };

    class ParameterEditorContextPrivate {
        Q_DECLARE_PUBLIC(ParameterEditorContext)

    public:
        ParameterEditorContext *q_ptr{};
        ProjectViewModelContext *projectContext{};
        Core::DspxDocument *document{};
        Core::SingerRegistry *registry{};
        QPointer<dspx::SingingClip> singingClip;
        QPointer<dspx::Sources> sources;
        QString architectureId;
        QString editingParameterId;
        QString referenceParameterId;
        QString editingDisplayName;
        QString referenceDisplayName;
        bool transformEditing{};
        bool restoreEditingSelectionAfterRefresh{};
        Core::ParameterInfo editingLastInfo;
        Core::ParameterInfo referenceLastInfo;

        ParameterDefinitionListModel *parameterModel{};
        ParameterViewModelBinding *pitchBinding{};
        ParameterViewModelBinding *editingBinding{};
        ParameterViewModelBinding *referenceBinding{};

        QVector<QMetaObject::Connection> clipConnections;
        QVector<QMetaObject::Connection> sourceConnections;

        void initialize();
        void setSingingClip(dspx::SingingClip *clip);
        void reconnectClip();
        void reconnectSources();
        void rebuildParameterModel();
        void refreshBindings();
        void abortEditingOperation();
        void clearParameterSelection();
        const ParameterDefinitionListModel::Entry *entry(const QString &parameterId) const;
        QString displayName(const QString &parameterId, const QString &remembered) const;
    };
}

#endif // DIFFSCOPE_VISUALEDITOR_PARAMETEREDITORCONTEXT_P_H
