// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#include "ItemSelectorDialog.h"

#include <algorithm>
#include <limits>
#include <optional>

#include <QAbstractItemModel>
#include <QAbstractItemView>
#include <QCheckBox>
#include <QContextMenuEvent>
#include <QFormLayout>
#include <QFrame>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QHideEvent>
#include <QItemSelectionModel>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QMouseEvent>
#include <QPointer>
#include <QQuickItem>
#include <QQuickWidget>
#include <QScrollArea>
#include <QScrollBar>
#include <QShowEvent>
#include <QSignalBlocker>
#include <QSizePolicy>
#include <QSpinBox>
#include <QStyle>
#include <QStyleOptionViewItem>
#include <QTimer>
#include <QTreeView>
#include <QVBoxLayout>

#include <CoreApi/runtimeinterface.h>

#include <SVSCraftCore/MusicTime.h>
#include <SVSCraftCore/MusicTimeline.h>
#include <SVSCraftGui/MusicTimeValidator.h>

#include <dspxmodelORM/AnchorNodeSequence.h>
#include <dspxmodelORM/Clip.h>
#include <dspxmodelORM/ClipSequence.h>
#include <dspxmodelORM/DynamicMixingAnchorSequence.h>
#include <dspxmodelORM/Model.h>
#include <dspxmodelORM/NoteSequence.h>
#include <dspxmodelORM/Parameter.h>
#include <dspxmodelORM/ParameterMap.h>
#include <dspxmodelORM/SingingClip.h>
#include <dspxmodelORM/Sources.h>
#include <dspxmodelORM/Track.h>
#include <dspxmodelORM/TrackList.h>
#include <dspxmodelSelectionModel/AnchorNodeSelectionModel.h>
#include <dspxmodelSelectionModel/ClipSelectionModel.h>
#include <dspxmodelSelectionModel/DynamicMixingAnchorSelectionModel.h>
#include <dspxmodelSelectionModel/NoteSelectionModel.h>
#include <dspxmodelSelectionModel/SelectionModel.h>

#include <coreplugin/DspxDocument.h>
#include <coreplugin/FreeParameterSelectionModel.h>
#include <coreplugin/ProjectDocumentContext.h>
#include <coreplugin/ProjectTimeline.h>
#include <coreplugin/ProjectWindowInterface.h>
#include <coreplugin/internal/ItemSelectorListModel.h>

namespace Core::Internal {

    namespace {

        struct ColumnText {
            QString title;
            QString listAccessibleName;
            QString selectAllAccessibleDescription;
        };

        struct DocumentSelectionContext {
            dspx::SelectionModel::SelectionType selectionType;
            QObject *container = nullptr;
        };

        bool isFreeParameterNode(NodeKind kind) {
            return kind == NodeKind::FreeformEdited
                   || kind == NodeKind::FreeformTransform;
        }

        FreeParameterSelectionModel::Layer freeParameterLayer(NodeKind kind) {
            return kind == NodeKind::FreeformTransform
                       ? FreeParameterSelectionModel::TransformLayer
                       : FreeParameterSelectionModel::EditedLayer;
        }

        QString freeParameterDisplayName(ItemSelectorListModel *model,
                                         bool transform) {
            const auto displayName = model ? model->parameterDisplayName()
                                           : QString{};
            return transform
                       ? ItemSelectorDialog::tr("%1 Transform").arg(displayName)
                       : displayName;
        }

        bool applyFreeParameterContext(NodeKind kind,
                                       ItemSelectorListModel *model,
                                       FreeParameterSelectionModel *selectionModel) {
            if (!isFreeParameterNode(kind)) {
                return false;
            }
            auto *parameter = model
                                  ? qobject_cast<dspx::Parameter *>(model->context())
                                  : nullptr;
            auto *parameterMap = parameter ? parameter->parameterMap() : nullptr;
            auto *clip = parameterMap ? parameterMap->singingClip() : nullptr;
            const auto parameterId = model ? model->contextKey() : QString{};
            if (clip && !parameterId.isEmpty()) {
                const auto layer = freeParameterLayer(kind);
                if (selectionModel->isActive()
                    && selectionModel->singingClip() == clip
                    && selectionModel->parameterId() == parameterId
                    && selectionModel->layer() == layer) {
                    return true;
                }
                selectionModel->setContext(
                    clip, parameterId,
                    freeParameterDisplayName(
                        model, kind == NodeKind::FreeformTransform),
                    layer);
            }
            return true;
        }

        bool isComponentNode(NodeKind kind) {
            switch (kind) {
                case NodeKind::RootTracks:
                case NodeKind::RootLabels:
                case NodeKind::RootTempos:
                case NodeKind::RootKeySignatures:
                case NodeKind::TrackClips:
                case NodeKind::SingingNotes:
                case NodeKind::SingingParameters:
                case NodeKind::SingingVoiceBlending:
                case NodeKind::FreeformEdited:
                case NodeKind::FreeformTransform:
                case NodeKind::EditedAnchors:
                case NodeKind::TransformAnchors:
                    return true;
                default:
                    return false;
            }
        }

        bool supportsDocumentContextMenu(const QModelIndex &index) {
            if (!index.isValid()) {
                return false;
            }
            const auto nodeKind = static_cast<NodeKind>(
                index.data(ItemSelectorListModel::NodeKindRole).toInt());
            return index.flags().testFlag(Qt::ItemIsUserCheckable)
                   || isComponentNode(nodeKind);
        }

        std::optional<DocumentSelectionContext> selectionContextForComponent(
            NodeKind kind, QObject *context) {
            switch (kind) {
                case NodeKind::RootTracks:
                    return DocumentSelectionContext{dspx::SelectionModel::ST_Track};
                case NodeKind::RootLabels:
                    return DocumentSelectionContext{dspx::SelectionModel::ST_Label};
                case NodeKind::RootTempos:
                    return DocumentSelectionContext{dspx::SelectionModel::ST_Tempo};
                case NodeKind::RootKeySignatures:
                    return DocumentSelectionContext{dspx::SelectionModel::ST_KeySignature};
                case NodeKind::TrackClips:
                    return DocumentSelectionContext{dspx::SelectionModel::ST_Clip};
                case NodeKind::SingingNotes: {
                    auto *clip = qobject_cast<dspx::SingingClip *>(context);
                    if (!clip) {
                        return std::nullopt;
                    }
                    return DocumentSelectionContext{
                        dspx::SelectionModel::ST_Note, clip->notes()};
                }
                case NodeKind::SingingVoiceBlending: {
                    auto *clip = qobject_cast<dspx::SingingClip *>(context);
                    if (!clip) {
                        return std::nullopt;
                    }
                    return DocumentSelectionContext{
                        dspx::SelectionModel::ST_DynamicMixingAnchor,
                        clip->sources()
                            ? clip->sources()->dynamicMixingAnchors()
                            : nullptr};
                }
                case NodeKind::EditedAnchors: {
                    auto *parameter = qobject_cast<dspx::Parameter *>(context);
                    if (!parameter) {
                        return std::nullopt;
                    }
                    return DocumentSelectionContext{
                        dspx::SelectionModel::ST_AnchorNode,
                        parameter->anchorEdited()};
                }
                case NodeKind::TransformAnchors: {
                    auto *parameter = qobject_cast<dspx::Parameter *>(context);
                    if (!parameter) {
                        return std::nullopt;
                    }
                    return DocumentSelectionContext{
                        dspx::SelectionModel::ST_AnchorNode,
                        parameter->anchorTransform()};
                }
                default:
                    return std::nullopt;
            }
        }

        void applySelectionContext(
            dspx::SelectionModel *selectionModel,
            const DocumentSelectionContext &context) {
            selectionModel->select(
                nullptr, dspx::SelectionModel::ClearPreviousSelection,
                context.selectionType, context.container);
        }

        ColumnText columnText(ListKind kind, QObject *context) {
            switch (kind) {
                case ListKind::Root:
                    return {
                        ItemSelectorDialog::tr("Document Components"),
                        ItemSelectorDialog::tr("Document Components List"),
                        ItemSelectorDialog::tr("Select all document components"),
                    };
                case ListKind::Tracks:
                    return {
                        ItemSelectorDialog::tr("Tracks"),
                        ItemSelectorDialog::tr("Tracks List"),
                        ItemSelectorDialog::tr("Select all tracks"),
                    };
                case ListKind::TrackBranches:
                    return {
                        ItemSelectorDialog::tr("Track Components"),
                        ItemSelectorDialog::tr("Track Components List"),
                        ItemSelectorDialog::tr("Select all track components"),
                    };
                case ListKind::Clips:
                    return {
                        ItemSelectorDialog::tr("Clips"),
                        ItemSelectorDialog::tr("Clips List"),
                        ItemSelectorDialog::tr("Select all clips"),
                    };
                case ListKind::SingingBranches:
                    return {
                        ItemSelectorDialog::tr("Clip Components"),
                        ItemSelectorDialog::tr("Clip Components List"),
                        ItemSelectorDialog::tr("Select all clip components"),
                    };
                case ListKind::Notes:
                    return {
                        ItemSelectorDialog::tr("Notes"),
                        ItemSelectorDialog::tr("Notes List"),
                        ItemSelectorDialog::tr("Select all notes"),
                    };
                case ListKind::Parameters:
                    return {
                        ItemSelectorDialog::tr("Parameters"),
                        ItemSelectorDialog::tr("Parameters List"),
                        ItemSelectorDialog::tr("Select all parameters"),
                    };
                case ListKind::AnchorBranches:
                    return {
                        ItemSelectorDialog::tr("Parameter Components"),
                        ItemSelectorDialog::tr("Parameter Components List"),
                        ItemSelectorDialog::tr("Select all parameter components"),
                    };
                case ListKind::Anchors: {
                    auto *sequence = qobject_cast<dspx::AnchorNodeSequence *>(context);
                    if (sequence && sequence->role() == dspx::AnchorNodeSequence::Transform) {
                        return {
                            ItemSelectorDialog::tr("Parameter Transform Anchors"),
                            ItemSelectorDialog::tr("Parameter Transform Anchors List"),
                            ItemSelectorDialog::tr("Select all parameter transform anchors"),
                        };
                    }
                    return {
                        ItemSelectorDialog::tr("Parameter Edited Anchors"),
                        ItemSelectorDialog::tr("Parameter Edited Anchors List"),
                        ItemSelectorDialog::tr("Select all parameter edited anchors"),
                    };
                }
                case ListKind::DynamicAnchors:
                    return {
                        ItemSelectorDialog::tr("Voice Blending Anchors"),
                        ItemSelectorDialog::tr("Voice Blending Anchors List"),
                        ItemSelectorDialog::tr("Select all voice blending anchors"),
                    };
                case ListKind::Labels:
                    return {
                        ItemSelectorDialog::tr("Labels"),
                        ItemSelectorDialog::tr("Labels List"),
                        ItemSelectorDialog::tr("Select all labels"),
                    };
                case ListKind::Tempos:
                    return {
                        ItemSelectorDialog::tr("Tempo Markings"),
                        ItemSelectorDialog::tr("Tempo Markings List"),
                        ItemSelectorDialog::tr("Select all tempo markings"),
                    };
                case ListKind::KeySignatures:
                    return {
                        ItemSelectorDialog::tr("Key Signatures"),
                        ItemSelectorDialog::tr("Key Signatures List"),
                        ItemSelectorDialog::tr("Select all key signatures"),
                    };
            }
            return {};
        }

        void toggleCheckState(const QModelIndex &index) {
            const auto current = static_cast<Qt::CheckState>(
                index.data(Qt::CheckStateRole).toInt()
            );
            const auto next = current == Qt::Checked ? Qt::Unchecked : Qt::Checked;
            const_cast<QAbstractItemModel *>(index.model())
                ->setData(index, next, Qt::CheckStateRole);
        }

        bool supportsDocumentCurrentItem(NodeKind kind) {
            switch (kind) {
                case NodeKind::Track:
                case NodeKind::Clip:
                case NodeKind::Note:
                case NodeKind::Anchor:
                case NodeKind::DynamicAnchor:
                case NodeKind::Label:
                case NodeKind::Tempo:
                case NodeKind::KeySignature:
                    return true;
                default:
                    return false;
            }
        }

        class ItemSelectorView : public QTreeView {
            Q_OBJECT

        public:
            explicit ItemSelectorView(QWidget *parent = nullptr) : QTreeView(parent) {
            }

        Q_SIGNALS:
            void focusPreviousRequested();
            void focusNextRequested();
            void itemContextMenuRequested(const QModelIndex &index);

        protected:
            void contextMenuEvent(QContextMenuEvent *event) override {
                const auto index = event->reason() == QContextMenuEvent::Keyboard
                                       ? currentIndex()
                                       : indexAt(event->pos());
                if (supportsDocumentContextMenu(index)) {
                    Q_EMIT itemContextMenuRequested(index);
                    event->accept();
                    return;
                }
                QTreeView::contextMenuEvent(event);
            }

            void mousePressEvent(QMouseEvent *event) override {
                const auto index = indexAt(event->position().toPoint());
                if (event->button() == Qt::RightButton
                    && supportsDocumentContextMenu(index)) {
                    m_contextMenuPressed = true;
                    event->accept();
                    return;
                }
                if (event->button() == Qt::LeftButton && index.isValid() && index.flags().testFlag(Qt::ItemIsUserCheckable) && checkIndicatorRect(index).contains(event->position().toPoint())) {
                    toggleCheckState(index);
                    m_checkboxPressed = true;
                    event->accept();
                    return;
                }
                QTreeView::mousePressEvent(event);
            }

            void mouseReleaseEvent(QMouseEvent *event) override {
                if (event->button() == Qt::RightButton
                    && m_contextMenuPressed) {
                    m_contextMenuPressed = false;
                    event->accept();
                    return;
                }
                if (m_checkboxPressed) {
                    m_checkboxPressed = false;
                    event->accept();
                    return;
                }
                QTreeView::mouseReleaseEvent(event);
            }

            void mouseDoubleClickEvent(QMouseEvent *event) override {
                const auto index = indexAt(event->position().toPoint());
                if (event->button() == Qt::LeftButton && index.isValid() && index.flags().testFlag(Qt::ItemIsUserCheckable) && checkIndicatorRect(index).contains(event->position().toPoint())) {
                    m_checkboxPressed = true;
                    event->accept();
                    return;
                }
                QTreeView::mouseDoubleClickEvent(event);
            }

            void keyPressEvent(QKeyEvent *event) override {
                if (event->key() == Qt::Key_Space && currentIndex().isValid() && currentIndex().flags().testFlag(Qt::ItemIsUserCheckable)) {
                    toggleCheckState(currentIndex());
                    event->accept();
                    return;
                }
                if (event->key() == Qt::Key_Left) {
                    if (isRightToLeft()) {
                        Q_EMIT focusNextRequested();
                    } else {
                        Q_EMIT focusPreviousRequested();
                    }
                    event->accept();
                    return;
                }
                if (event->key() == Qt::Key_Right) {
                    if (isRightToLeft()) {
                        Q_EMIT focusPreviousRequested();
                    } else {
                        Q_EMIT focusNextRequested();
                    }
                    event->accept();
                    return;
                }
                QTreeView::keyPressEvent(event);
            }

        private:
            QRect checkIndicatorRect(const QModelIndex &index) const {
                QStyleOptionViewItem option;
                option.initFrom(this);
                option.index = index;
                option.rect = visualRect(index);
                option.features |= QStyleOptionViewItem::HasCheckIndicator;
                option.checkState = static_cast<Qt::CheckState>(
                    index.data(Qt::CheckStateRole).toInt()
                );
                return style()->subElementRect(
                    QStyle::SE_ItemViewItemCheckIndicator, &option, this
                );
            }

            bool m_checkboxPressed = false;
            bool m_contextMenuPressed = false;
        };

        class SelectAllCheckBox : public QCheckBox {
        public:
            explicit SelectAllCheckBox(QWidget *parent = nullptr) : QCheckBox(parent) {
                setTristate(true);
            }

        protected:
            void nextCheckState() override {
                setCheckState(checkState() == Qt::Checked ? Qt::Unchecked : Qt::Checked);
            }
        };

        class MusicTimeSpinBox : public QSpinBox {
        public:
            explicit MusicTimeSpinBox(SVS::MusicTimeline *timeline,
                                      QWidget *parent = nullptr)
                : QSpinBox(parent), m_timeline(timeline),
                  m_validator(new SVS::MusicTimeValidator(this)) {
                setRange(0, std::numeric_limits<int>::max());
                m_validator->setRange(minimum(), maximum());
                m_validator->setTimeline(m_timeline);
                if (m_timeline) {
                    connect(m_timeline, &SVS::MusicTimeline::changed,
                            this, &MusicTimeSpinBox::refreshText);
                }
            }

            void setMusicValue(int value) {
                setValue(value);
                refreshText();
            }

        protected:
            QString textFromValue(int value) const override {
                return m_timeline
                           ? m_timeline->create(0, 0, value).toString(1, 1, 3)
                           : QString{};
            }

            int valueFromText(const QString &text) const override {
                if (!m_timeline) {
                    return value();
                }
                bool ok = false;
                const int result = m_timeline->create(text, &ok).totalTick();
                return ok ? result : value();
            }

            QValidator::State validate(QString &input, int &position) const override {
                return m_validator->validate(input, position);
            }

            void fixup(QString &input) const override {
                m_validator->fixup(input);
            }

        private:
            void refreshText() {
                if (!lineEdit()) {
                    return;
                }
                const int cursorPosition = lineEdit()->cursorPosition();
                const QSignalBlocker blocker(this);
                lineEdit()->setText(textFromValue(value()));
                lineEdit()->setCursorPosition(
                    std::min(cursorPosition,
                             static_cast<int>(lineEdit()->text().size())));
                updateGeometry();
            }

            SVS::MusicTimeline *m_timeline;
            SVS::MusicTimeValidator *m_validator;
        };

    }

    void ItemSelectorDialog::initialize() {
        setWindowTitle(ItemSelectorDialog::tr("Item Selector"));
        setWindowModality(Qt::NonModal);
        setWindowFlag(Qt::Tool);
        resize(960, 620);

        auto menuLayerWindow = new QQuickWidget(RuntimeInterface::qmlEngine(), this);
        menuLayerWindow->setInitialProperties({
            {"windowHandle", QVariant::fromValue(m_windowInterface)}
        });
        menuLayerWindow->loadFromModule("DiffScope.Core", "ItemSelectorMenuLayer");
        if (menuLayerWindow->status() == QQuickWidget::Status::Error || !menuLayerWindow->rootObject()) {
            qFatal() << menuLayerWindow->errors();
        }
        m_menuLayerObject = menuLayerWindow->rootObject();

        auto *dialogLayout = new QVBoxLayout(this);
        m_scrollArea = new QScrollArea(this);
        m_scrollArea->setWidgetResizable(true);
        m_scrollArea->setFrameShape(QFrame::NoFrame);
        m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        m_scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        m_columnsWidget = new QWidget(m_scrollArea);
        m_columnsWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);
        m_columnsLayout = new QHBoxLayout(m_columnsWidget);
        m_columnsLayout->setContentsMargins(0, 0, 0, 0);
        m_columnsLayout->setSpacing(4);
        m_columnsLayout->setAlignment(Qt::AlignLeft);
        m_scrollArea->setWidget(m_columnsWidget);
        dialogLayout->addWidget(m_scrollArea, 1);
    }

    void ItemSelectorDialog::prepareToShow() {
        if (m_columns.isEmpty()) {
            const bool updatesWereEnabled = updatesEnabled();
            if (updatesWereEnabled) {
                setUpdatesEnabled(false);
            }
            addColumn(ListKind::Root, model());
            restoreInitialPath();
            if (updatesWereEnabled) {
                setUpdatesEnabled(true);
                update();
            }
            focusInitialColumn();
        }
    }

    void ItemSelectorDialog::releaseColumns() {
        ++m_horizontalScrollRestoreGeneration;
        removeColumnsAfter(-1);
        m_scrollArea->horizontalScrollBar()->setValue(0);
    }

    dspx::Model *ItemSelectorDialog::model() const {
        return m_windowInterface->projectDocumentContext()->document()->model();
    }

    dspx::SelectionModel *ItemSelectorDialog::selectionModel() const {
        return m_windowInterface->projectDocumentContext()->document()->selectionModel();
    }

    FreeParameterSelectionModel *ItemSelectorDialog::freeParameterSelectionModel() const {
        return m_windowInterface->projectDocumentContext()->document()
            ->freeParameterSelectionModel();
    }

    void ItemSelectorDialog::popupDocumentContextMenu(
        dspx::SelectionModel::SelectionType selectionType,
        bool sceneContextMenu) {
        QMetaObject::invokeMethod(
            m_menuLayerObject, "popupDocumentContextMenu",
            Q_ARG(QVariant, selectionType),
            Q_ARG(QVariant, sceneContextMenu));
    }

    void ItemSelectorDialog::addColumn(ListKind kind, QObject *context,
                                       const QString &contextKey) {
        const auto text = columnText(kind, context);
        auto *columnWidget = new QGroupBox(text.title, m_columnsWidget);
        columnWidget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
        auto *layout = new QVBoxLayout(columnWidget);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(4);

        auto *model = new ItemSelectorListModel(
            kind, context, contextKey, m_windowInterface, columnWidget);
        auto *selectAll = new SelectAllCheckBox(columnWidget);
        selectAll->setText(ItemSelectorDialog::tr("Select All"));
        selectAll->setAccessibleDescription(text.selectAllAccessibleDescription);
        selectAll->setVisible(model->isSelectableList());
        layout->addWidget(selectAll);

        auto *view = new ItemSelectorView(columnWidget);
        view->setAccessibleName(text.listAccessibleName);
        view->setModel(model);
        view->setHeaderHidden(true);
        view->header()->setStretchLastSection(false);
        view->header()->setSectionResizeMode(0, QHeaderView::Fixed);
        view->setRootIsDecorated(false);
        view->setItemsExpandable(false);
        view->setIndentation(0);
        view->setSelectionBehavior(QAbstractItemView::SelectRows);
        view->setSelectionMode(QAbstractItemView::SingleSelection);
        view->setEditTriggers(QAbstractItemView::NoEditTriggers);
        view->setAllColumnsShowFocus(true);
        view->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        view->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
        view->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
        layout->addWidget(view, 1);

        m_columnsLayout->addWidget(columnWidget);
        Column column;
        column.widget = columnWidget;
        column.selectAll = selectAll;
        column.focusWidget = view;
        column.view = view;
        column.model = model;
        column.context = context;
        column.contextKey = contextKey;
        m_columns.append(column);
        updateColumnWidth(columnWidget);

        const auto updateSelectAll = [model, selectAll] {
            const QSignalBlocker blocker(selectAll);
            selectAll->setEnabled(model->hasSelectableItems());
            selectAll->setCheckState(model->checkStateSummary());
        };
        updateSelectAll();
        QObject::connect(model, &ItemSelectorListModel::checkStateSummaryChanged, this, updateSelectAll);
        QObject::connect(model, &QAbstractItemModel::rowsInserted, this, [this, columnWidget, updateSelectAll] {
            updateSelectAll();
            updateColumnWidth(columnWidget);
        });
        QObject::connect(model, &QAbstractItemModel::rowsRemoved, this, [this, columnWidget, updateSelectAll] {
            updateSelectAll();
            updateColumnWidth(columnWidget);
        });
        QObject::connect(model, &QAbstractItemModel::modelReset, this, [this, columnWidget] {
            updateColumnWidth(columnWidget);
        });
        QObject::connect(model, &QAbstractItemModel::dataChanged, this, [this, columnWidget](const QModelIndex &, const QModelIndex &, const QList<int> &roles) {
            if (roles.isEmpty() || roles.contains(Qt::DisplayRole)) {
                updateColumnWidth(columnWidget);
            }
        });
        QObject::connect(selectAll, &QCheckBox::clicked, this, [model, selectAll] {
            model->setAllSelected(selectAll->checkState() == Qt::Checked);
        });

        QObject::connect(view->selectionModel(), &QItemSelectionModel::currentChanged, this, [this, columnWidget](const QModelIndex &current) {
            if (m_mutatingColumns) {
                return;
            }
            const int columnIndex = indexOfColumn(columnWidget);
            if (columnIndex >= 0) {
                navigateFrom(columnIndex, current);
            }
        });
        QObject::connect(view, &QAbstractItemView::activated, this, [this, columnWidget, model](const QModelIndex &index) {
            const auto *sourceEntry = index.isValid()
                                          ? model->entryAt(index.row())
                                          : nullptr;
            const ItemSelectorEntry entry = sourceEntry
                                                ? *sourceEntry
                                                : ItemSelectorEntry{};
            if (!sourceEntry) {
                return;
            }

            const int columnIndex = indexOfColumn(columnWidget);
            if (columnIndex < 0) {
                return;
            }
            navigateFrom(columnIndex, index);

            if (applyFreeParameterContext(
                    entry.kind, model, freeParameterSelectionModel())) {
                return;
            }

            if (const auto context = selectionContextForComponent(
                    entry.kind, model->context())) {
                applySelectionContext(selectionModel(), *context);
                return;
            }

            QObject *item = entry.object.data();
            auto *selection = selectionModel();
            if (!supportsDocumentCurrentItem(entry.kind) || !item || selection->currentItem() == item) {
                return;
            }
            selection->select(item, dspx::SelectionModel::SetCurrentItem);
        });
        QObject::connect(view, &ItemSelectorView::itemContextMenuRequested, this, [this, model](const QModelIndex &index) {
            const auto *entry = model->entryAt(index.row());
            if (!entry) {
                return;
            }

            if (applyFreeParameterContext(
                    entry->kind, model, freeParameterSelectionModel())) {
                return;
            }

            if (const auto context = selectionContextForComponent(
                    entry->kind, model->context())) {
                applySelectionContext(selectionModel(), *context);
                popupDocumentContextMenu(context->selectionType, true);
                return;
            }

            QObject *item = entry->object.data();
            const auto selectionType =
                dspx::SelectionModel::selectionTypeFromItem(item);
            if (!item || selectionType == dspx::SelectionModel::ST_None) {
                return;
            }

            auto *selection = selectionModel();
            selection->select(
                item,
                dspx::SelectionModel::ClearPreviousSelection
                    | dspx::SelectionModel::Select
                    | dspx::SelectionModel::SetCurrentItem
            );
            popupDocumentContextMenu(selectionType, false);
        });
        QObject::connect(model, &QAbstractItemModel::rowsAboutToBeRemoved, this, [this, columnWidget, view](const QModelIndex &, int first, int last) {
            const auto current = view->currentIndex();
            if (!current.isValid() || current.row() < first || current.row() > last) {
                return;
            }
            const bool wasMutatingColumns = m_mutatingColumns;
            m_mutatingColumns = true;
            view->setCurrentIndex({});
            m_mutatingColumns = wasMutatingColumns;
            const int columnIndex = indexOfColumn(columnWidget);
            if (columnIndex >= 0) {
                removeColumnsAfterPreservingScroll(columnIndex);
            }
        });
        QObject::connect(model, &QAbstractItemModel::modelAboutToBeReset, this, [this, columnWidget, view] {
            if (!view->currentIndex().isValid()) {
                return;
            }
            const bool wasMutatingColumns = m_mutatingColumns;
            m_mutatingColumns = true;
            view->setCurrentIndex({});
            m_mutatingColumns = wasMutatingColumns;
            const int columnIndex = indexOfColumn(columnWidget);
            if (columnIndex >= 0) {
                removeColumnsAfterPreservingScroll(columnIndex);
            }
        });
        QObject::connect(view, &ItemSelectorView::focusPreviousRequested, this, [this, columnWidget] {
            const int index = indexOfColumn(columnWidget);
            if (index > 0) {
                focusColumn(index - 1, true);
            }
        });
        QObject::connect(view, &ItemSelectorView::focusNextRequested, this, [this, columnWidget] {
            const int index = indexOfColumn(columnWidget);
            if (index >= 0 && index + 1 < m_columns.size()) {
                focusColumn(index + 1, true);
            }
        });
    }

    void ItemSelectorDialog::addRangeColumn(
        dspx::SingingClip *clip,
        ItemSelectorListModel *parameterComponentsModel,
        bool transform) {
        if (!clip || !parameterComponentsModel
            || parameterComponentsModel->contextKey().isEmpty()) {
            return;
        }

        auto *columnWidget = new QGroupBox(
            ItemSelectorDialog::tr("Range"), m_columnsWidget);
        columnWidget->setAccessibleName(ItemSelectorDialog::tr("Range"));
        columnWidget->setCheckable(true);
        columnWidget->setFocusPolicy(Qt::StrongFocus);
        columnWidget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);

        auto *layout = new QFormLayout(columnWidget);
        layout->setContentsMargins(0, 0, 0, 0);

        auto *timeline = m_windowInterface->projectTimeline()->musicTimeline();
        auto *startSpinBox = new MusicTimeSpinBox(timeline, columnWidget);
        auto *endSpinBox = new MusicTimeSpinBox(timeline, columnWidget);
        startSpinBox->setAccessibleName(ItemSelectorDialog::tr("Start"));
        endSpinBox->setAccessibleName(ItemSelectorDialog::tr("End"));

        auto *startLabel = new QLabel(ItemSelectorDialog::tr("Start"), columnWidget);
        auto *endLabel = new QLabel(ItemSelectorDialog::tr("End"), columnWidget);
        startLabel->setBuddy(startSpinBox);
        endLabel->setBuddy(endSpinBox);
        layout->addRow(startLabel, startSpinBox);
        layout->addRow(endLabel, endSpinBox);

        m_columnsLayout->addWidget(columnWidget);
        Column column;
        column.widget = columnWidget;
        column.focusWidget = columnWidget;
        column.rangeColumn = true;
        column.context = clip;
        column.contextKey = parameterComponentsModel->contextKey();
        column.transform = transform;
        m_columns.append(column);
        updateColumnWidth(columnWidget);

        const QPointer<dspx::SingingClip> guardedClip = clip;
        const QPointer<ItemSelectorListModel> guardedParameterModel =
            parameterComponentsModel;
        auto *freeSelection = freeParameterSelectionModel();
        const auto layer = transform
                               ? FreeParameterSelectionModel::TransformLayer
                               : FreeParameterSelectionModel::EditedLayer;
        const QString parameterId = parameterComponentsModel->contextKey();
        const auto contextMatches = [freeSelection, guardedClip,
                                     parameterId, layer] {
            return guardedClip && freeSelection->isActive()
                   && freeSelection->singingClip() == guardedClip
                   && freeSelection->parameterId() == parameterId
                   && freeSelection->layer() == layer;
        };
        const auto synchronizeRange = [columnWidget, startSpinBox, endSpinBox,
                                       freeSelection, contextMatches] {
            const bool matches = contextMatches();
            const QSignalBlocker groupBlocker(columnWidget);
            const QSignalBlocker startBlocker(startSpinBox);
            const QSignalBlocker endBlocker(endSpinBox);
            columnWidget->setChecked(matches && freeSelection->hasSelection());
            startSpinBox->setMusicValue(matches ? freeSelection->start() : 0);
            endSpinBox->setMusicValue(matches ? freeSelection->end() : 0);
        };

        QObject::connect(freeSelection, &FreeParameterSelectionModel::contextChanged,
                         columnWidget, synchronizeRange);
        QObject::connect(freeSelection, &FreeParameterSelectionModel::activeChanged,
                         columnWidget, synchronizeRange);
        QObject::connect(freeSelection, &FreeParameterSelectionModel::rangeChanged,
                         columnWidget, synchronizeRange);
        QObject::connect(freeSelection, &FreeParameterSelectionModel::hasSelectionChanged,
                         columnWidget, synchronizeRange);
        QObject::connect(columnWidget, &QGroupBox::clicked, columnWidget,
                         [freeSelection, guardedClip, guardedParameterModel,
                          parameterId, layer, contextMatches,
                          synchronizeRange](bool checked) {
            if (!checked) {
                if (contextMatches()) {
                    freeSelection->clear();
                }
                return;
            }
            if (!guardedClip || !guardedParameterModel) {
                synchronizeRange();
                return;
            }
            freeSelection->setContext(
                guardedClip, parameterId,
                freeParameterDisplayName(
                    guardedParameterModel, layer == FreeParameterSelectionModel::TransformLayer),
                layer);
            freeSelection->setRange(
                guardedClip->clipStart(),
                guardedClip->clipStart() + guardedClip->clipLength());
            synchronizeRange();
        });

        const auto updateSelectionRange = [freeSelection, contextMatches,
                                           startSpinBox, endSpinBox] {
            if (!contextMatches()) {
                return;
            }
            freeSelection->setRange(startSpinBox->value(), endSpinBox->value());
        };
        QObject::connect(startSpinBox, &QSpinBox::valueChanged,
                         columnWidget, updateSelectionRange);
        QObject::connect(endSpinBox, &QSpinBox::valueChanged,
                         columnWidget, updateSelectionRange);
        QObject::connect(timeline, &SVS::MusicTimeline::changed,
                         columnWidget, [this, columnWidget] {
            const int horizontalScrollValue =
                m_scrollArea->horizontalScrollBar()->value();
            updateColumnWidth(columnWidget);
            restoreHorizontalScrollPosition(horizontalScrollValue);
        });

        synchronizeRange();
    }

    void ItemSelectorDialog::updateColumnWidth(QWidget *columnWidget) {
        const int columnIndex = indexOfColumn(columnWidget);
        if (columnIndex < 0) {
            return;
        }

        constexpr int minimumColumnWidth = 160;
        constexpr int maximumColumnWidth = 280;
        constexpr int maximumSampleCount = 120;

        const QPointer<QGroupBox> column = m_columns.at(columnIndex).widget;
        if (!column) {
            return;
        }
        if (m_columns.at(columnIndex).rangeColumn) {
            const int targetWidth = std::max(
                column->minimumSizeHint().width(),
                std::clamp(column->sizeHint().width(), minimumColumnWidth,
                           maximumColumnWidth));
            column->setFixedWidth(targetWidth);
            m_columnsLayout->invalidate();
            m_columnsLayout->activate();
            return;
        }

        const QPointer<QCheckBox> selectAll = m_columns.at(columnIndex).selectAll;
        const QPointer<QTreeView> view = m_columns.at(columnIndex).view;
        const QPointer<ItemSelectorListModel> model = m_columns.at(columnIndex).model;
        if (!selectAll || !view || !model) {
            return;
        }

        int contentWidth = 0;
        const int rowCount = model->rowCount();
        const int sampleCount = std::min(rowCount, maximumSampleCount);
        for (int sample = 0; sample < sampleCount; ++sample) {
            const int row = sampleCount < 2
                                ? 0
                                : sample * (rowCount - 1) / (sampleCount - 1);
            contentWidth = std::max(
                contentWidth,
                view->sizeHintForIndex(model->index(row)).width()
            );
        }

        const auto layoutMargins = column->layout()->contentsMargins();
        const auto groupMargins = column->contentsMargins();
        const int horizontalLayoutMargins = layoutMargins.left() + layoutMargins.right() + groupMargins.left() + groupMargins.right();
        const int contentPreferredWidth = contentWidth + view->frameWidth() * 2 + horizontalLayoutMargins + 12;
        const int selectAllPreferredWidth = selectAll->isVisible()
                                                ? selectAll->sizeHint().width() + horizontalLayoutMargins
                                                : 0;
        const int groupTitlePreferredWidth = column->minimumSizeHint().width();

        const int horizontalScrollValue = view->horizontalScrollBar()->value();
        const auto horizontalScrollBarPolicy = view->horizontalScrollBarPolicy();
        const bool updatesWereEnabled = view->updatesEnabled();
        if (updatesWereEnabled) {
            view->setUpdatesEnabled(false);
        }
        view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

        const auto applyColumnWidth = [this, column, view](int width) {
            column->setFixedWidth(width);
            m_columnsLayout->invalidate();
            m_columnsLayout->activate();
            if (column->layout()) {
                column->layout()->invalidate();
                column->layout()->activate();
            }
            view->doItemsLayout();
        };

        int targetWidth = std::max(
            groupTitlePreferredWidth,
            std::clamp(std::max(contentPreferredWidth, selectAllPreferredWidth), minimumColumnWidth, maximumColumnWidth)
        );
        applyColumnWidth(targetWidth);
        if (view->verticalScrollBar()->maximum() > view->verticalScrollBar()->minimum()) {
            const int scrollBarExtent = view->style()->pixelMetric(
                QStyle::PM_ScrollBarExtent, nullptr, view
            );
            targetWidth = std::max(
                groupTitlePreferredWidth,
                std::clamp(std::max(contentPreferredWidth + scrollBarExtent, selectAllPreferredWidth), minimumColumnWidth, maximumColumnWidth)
            );
            applyColumnWidth(targetWidth);
        }

        if (contentWidth <= view->viewport()->width()) {
            view->header()->setSectionResizeMode(0, QHeaderView::Stretch);
        } else {
            view->header()->setSectionResizeMode(0, QHeaderView::Fixed);
            view->setColumnWidth(0, contentWidth);
        }
        view->doItemsLayout();
        view->setHorizontalScrollBarPolicy(horizontalScrollBarPolicy);
        view->doItemsLayout();
        view->horizontalScrollBar()->setValue(
            std::min(horizontalScrollValue, view->horizontalScrollBar()->maximum())
        );
        if (updatesWereEnabled) {
            view->setUpdatesEnabled(true);
            view->update();
        }
    }

    void ItemSelectorDialog::focusInitialColumn() {
        if (m_columns.isEmpty()) {
            return;
        }
        const QPointer<QWidget> column = m_columns.constLast().widget;
        const QPointer<QWidget> focusWidget = m_columns.constLast().focusWidget;
        QTimer::singleShot(0, this, [this, column, focusWidget] {
            if (!column || !focusWidget || !isVisible()) {
                return;
            }
            const bool updatesWereEnabled = updatesEnabled();
            if (updatesWereEnabled) {
                setUpdatesEnabled(false);
            }
            const auto columnSnapshot = m_columns;
            for (const auto &currentColumn : columnSnapshot) {
                if (currentColumn.widget) {
                    updateColumnWidth(currentColumn.widget);
                }
            }
            if (updatesWereEnabled) {
                setUpdatesEnabled(true);
                update();
            }
            focusWidget->setFocus();
            m_scrollArea->ensureWidgetVisible(column, m_columnsLayout->spacing(), 0);
        });
    }

    void ItemSelectorDialog::focusColumn(int columnIndex, bool ensureVisible) {
        if (columnIndex < 0 || columnIndex >= m_columns.size()) {
            return;
        }
        const QPointer<QWidget> column = m_columns.at(columnIndex).widget;
        const QPointer<QWidget> focusWidget = m_columns.at(columnIndex).focusWidget;
        if (!column || !focusWidget) {
            return;
        }
        focusWidget->setFocus();
        if (ensureVisible) {
            m_scrollArea->ensureWidgetVisible(column, m_columnsLayout->spacing(), 0);
        }
    }

    void ItemSelectorDialog::restoreHorizontalScrollPosition(int value) {
        const int generation = ++m_horizontalScrollRestoreGeneration;
        const QPointer<QScrollBar> scrollBar = m_scrollArea->horizontalScrollBar();
        if (!scrollBar) {
            return;
        }
        scrollBar->setValue(std::clamp(value, scrollBar->minimum(), scrollBar->maximum()));
        QTimer::singleShot(0, this, [this, generation, scrollBar, value] {
            if (generation != m_horizontalScrollRestoreGeneration || !scrollBar) {
                return;
            }
            scrollBar->setValue(std::clamp(value, scrollBar->minimum(), scrollBar->maximum()));
        });
    }

    void ItemSelectorDialog::removeColumnsAfterPreservingScroll(int columnIndex) {
        const int horizontalScrollValue = m_scrollArea->horizontalScrollBar()->value();
        const bool updatesWereEnabled = updatesEnabled();
        if (updatesWereEnabled) {
            setUpdatesEnabled(false);
        }
        removeColumnsAfter(columnIndex);
        m_columnsWidget->updateGeometry();
        m_columnsLayout->invalidate();
        m_columnsLayout->activate();
        if (updatesWereEnabled) {
            setUpdatesEnabled(true);
            update();
        }
        restoreHorizontalScrollPosition(horizontalScrollValue);
    }

    void ItemSelectorDialog::navigateFrom(int columnIndex, const QModelIndex &current) {
        if (columnIndex < 0 || columnIndex >= m_columns.size()) {
            return;
        }

        bool hasNextColumn = false;
        bool nextIsRangeColumn = false;
        bool nextRangeTransform = false;
        ListKind nextKind = ListKind::Root;
        QObject *nextContext = nullptr;
        QString nextContextKey;

        const QPointer<ItemSelectorListModel> columnModel = m_columns.at(columnIndex).model;
        if (!columnModel) {
            return;
        }
        const auto *sourceEntry = current.isValid() ? columnModel->entryAt(current.row()) : nullptr;
        const ItemSelectorEntry entry = sourceEntry ? *sourceEntry : ItemSelectorEntry{};

        if (sourceEntry) {
            switch (entry.kind) {
                case NodeKind::RootTracks:
                    hasNextColumn = true;
                    nextKind = ListKind::Tracks;
                    nextContext = model();
                    break;
                case NodeKind::RootLabels:
                    hasNextColumn = true;
                    nextKind = ListKind::Labels;
                    nextContext = model();
                    break;
                case NodeKind::RootTempos:
                    hasNextColumn = true;
                    nextKind = ListKind::Tempos;
                    nextContext = model();
                    break;
                case NodeKind::RootKeySignatures:
                    hasNextColumn = true;
                    nextKind = ListKind::KeySignatures;
                    nextContext = model();
                    break;
                case NodeKind::Track:
                    hasNextColumn = true;
                    nextKind = ListKind::TrackBranches;
                    nextContext = entry.object.data();
                    break;
                case NodeKind::TrackClips:
                    hasNextColumn = true;
                    nextKind = ListKind::Clips;
                    nextContext = columnModel->context();
                    break;
                case NodeKind::Clip: {
                    auto *clip = qobject_cast<dspx::Clip *>(entry.object.data());
                    if (clip && clip->type() == dspx::Clip::Singing) {
                        hasNextColumn = true;
                        nextKind = ListKind::SingingBranches;
                        nextContext = clip;
                    }
                    break;
                }
                case NodeKind::SingingNotes:
                    hasNextColumn = true;
                    nextKind = ListKind::Notes;
                    nextContext = columnModel->context();
                    break;
                case NodeKind::SingingParameters:
                    hasNextColumn = true;
                    nextKind = ListKind::Parameters;
                    nextContext = columnModel->context();
                    break;
                case NodeKind::SingingVoiceBlending:
                    hasNextColumn = true;
                    nextKind = ListKind::DynamicAnchors;
                    nextContext = columnModel->context();
                    break;
                case NodeKind::Parameter:
                    hasNextColumn = true;
                    nextKind = ListKind::AnchorBranches;
                    nextContext = entry.object.data();
                    nextContextKey = entry.key;
                    break;
                case NodeKind::FreeformEdited:
                case NodeKind::FreeformTransform: {
                    auto *parameter = qobject_cast<dspx::Parameter *>(
                        columnModel->context());
                    auto *parameterMap = parameter
                                             ? parameter->parameterMap()
                                             : nullptr;
                    auto *clip = parameterMap
                                     ? parameterMap->singingClip()
                                     : nullptr;
                    if (clip && !columnModel->contextKey().isEmpty()) {
                        hasNextColumn = true;
                        nextIsRangeColumn = true;
                        nextRangeTransform =
                            entry.kind == NodeKind::FreeformTransform;
                        nextContext = clip;
                        nextContextKey = columnModel->contextKey();
                    }
                    break;
                }
                case NodeKind::EditedAnchors: {
                    auto *parameter = qobject_cast<dspx::Parameter *>(columnModel->context());
                    hasNextColumn = true;
                    nextKind = ListKind::Anchors;
                    nextContext = parameter ? parameter->anchorEdited() : nullptr;
                    nextContextKey = columnModel->contextKey();
                    break;
                }
                case NodeKind::TransformAnchors: {
                    auto *parameter = qobject_cast<dspx::Parameter *>(columnModel->context());
                    hasNextColumn = true;
                    nextKind = ListKind::Anchors;
                    nextContext = parameter ? parameter->anchorTransform() : nullptr;
                    nextContextKey = columnModel->contextKey();
                    break;
                }
                default:
                    break;
            }
        }

        const int nextColumnIndex = columnIndex + 1;
        if (hasNextColumn && nextColumnIndex < m_columns.size()) {
            const auto &nextColumn = m_columns.at(nextColumnIndex);
            if (nextIsRangeColumn) {
                if (nextColumn.rangeColumn
                    && nextColumn.context == nextContext
                    && nextColumn.contextKey == nextContextKey
                    && nextColumn.transform == nextRangeTransform) {
                    return;
                }
            } else {
                const QPointer<ItemSelectorListModel> nextColumnModel =
                    nextColumn.model;
                if (nextColumnModel
                    && nextColumnModel->listKind() == nextKind
                    && nextColumnModel->context() == nextContext
                    && nextColumnModel->contextKey() == nextContextKey) {
                    return;
                }
            }
        } else if (!hasNextColumn && nextColumnIndex == m_columns.size()) {
            return;
        }

        const int horizontalScrollValue = m_scrollArea->horizontalScrollBar()->value();
        const bool updatesWereEnabled = updatesEnabled();
        if (updatesWereEnabled) {
            setUpdatesEnabled(false);
        }
        removeColumnsAfter(columnIndex);
        if (hasNextColumn) {
            if (nextIsRangeColumn) {
                addRangeColumn(
                    qobject_cast<dspx::SingingClip *>(nextContext),
                    columnModel, nextRangeTransform);
            } else {
                addColumn(nextKind, nextContext, nextContextKey);
            }
        }
        m_columnsWidget->updateGeometry();
        m_columnsLayout->invalidate();
        m_columnsLayout->activate();
        if (updatesWereEnabled) {
            setUpdatesEnabled(true);
            update();
        }
        restoreHorizontalScrollPosition(horizontalScrollValue);
    }

    void ItemSelectorDialog::removeColumnsAfter(int columnIndex) {
        if (columnIndex < -1) {
            return;
        }
        const bool wasMutatingColumns = m_mutatingColumns;
        m_mutatingColumns = true;
        while (m_columns.size() > columnIndex + 1) {
            auto column = m_columns.takeLast();
            m_columnsLayout->removeWidget(column.widget);
            delete column.widget;
        }
        m_mutatingColumns = wasMutatingColumns;
    }

    int ItemSelectorDialog::indexOfColumn(QWidget *widget) const {
        for (int index = 0; index < m_columns.size(); ++index) {
            if (m_columns.at(index).widget == widget) {
                return index;
            }
        }
        return -1;
    }

    bool ItemSelectorDialog::activate(int columnIndex, NodeKind kind, QObject *object, const QString &key) {
        if (columnIndex < 0 || columnIndex >= m_columns.size()) {
            return false;
        }
        const QPointer<ItemSelectorListModel> columnModel = m_columns.at(columnIndex).model;
        const QPointer<QTreeView> columnView = m_columns.at(columnIndex).view;
        if (!columnModel || !columnView) {
            return false;
        }
        const int row = columnModel->rowFor(kind, object, key);
        if (row < 0) {
            return false;
        }
        const int horizontalScrollValue = columnView->horizontalScrollBar()->value();
        const auto target = columnModel->index(row);
        columnView->setCurrentIndex(target);
        if (!columnView || !columnModel) {
            return false;
        }
        columnView->scrollTo(target, QAbstractItemView::PositionAtCenter);
        columnView->horizontalScrollBar()->setValue(
            std::min(horizontalScrollValue, columnView->horizontalScrollBar()->maximum())
        );
        return true;
    }

    void ItemSelectorDialog::restoreInitialPath() {
        if (freeParameterSelectionModel()->isActive()) {
            restoreFreeParameterPath();
            return;
        }
        auto *selection = selectionModel();
        switch (selection->selectionType()) {
            case dspx::SelectionModel::ST_None:
                return;
            case dspx::SelectionModel::ST_Track:
                activate(0, NodeKind::RootTracks);
                return;
            case dspx::SelectionModel::ST_Label:
                activate(0, NodeKind::RootLabels);
                return;
            case dspx::SelectionModel::ST_Tempo:
                activate(0, NodeKind::RootTempos);
                return;
            case dspx::SelectionModel::ST_KeySignature:
                activate(0, NodeKind::RootKeySignatures);
                return;
            case dspx::SelectionModel::ST_Clip:
                restoreClipPath();
                return;
            case dspx::SelectionModel::ST_Note:
                restoreNotePath();
                return;
            case dspx::SelectionModel::ST_AnchorNode:
                restoreAnchorPath();
                return;
            case dspx::SelectionModel::ST_DynamicMixingAnchor:
                restoreDynamicAnchorPath();
                return;
        }
    }

    void ItemSelectorDialog::restoreClipPath() {
        auto *clipSelection = selectionModel()->clipSelectionModel();
        dspx::Track *targetTrack = nullptr;
        int targetTrackIndex = std::numeric_limits<int>::max();
        const auto tracks = model()->tracks()->items();
        for (auto *clip : clipSelection->selectedItems()) {
            auto *track = clip && clip->clipSequence() ? clip->clipSequence()->track() : nullptr;
            const int index = tracks.indexOf(track);
            if (index >= 0 && index < targetTrackIndex) {
                targetTrack = track;
                targetTrackIndex = index;
            }
        }
        if (!targetTrack) {
            auto *clip = clipSelection->currentItem();
            targetTrack = clip && clip->clipSequence() ? clip->clipSequence()->track() : nullptr;
        }
        if (!targetTrack || !activate(0, NodeKind::RootTracks)) {
            return;
        }
        if (activate(1, NodeKind::Track, targetTrack)) {
            activate(2, NodeKind::TrackClips);
        }
    }

    void ItemSelectorDialog::restoreNotePath() {
        auto *sequence = selectionModel()->noteSelectionModel()->noteSequenceWithSelectedItems();
        auto *clip = sequence ? sequence->singingClip() : nullptr;
        auto *track = clip && clip->clipSequence() ? clip->clipSequence()->track() : nullptr;
        if (!track || !activate(0, NodeKind::RootTracks) || !activate(1, NodeKind::Track, track) || !activate(2, NodeKind::TrackClips) || !activate(3, NodeKind::Clip, clip)) {
            return;
        }
        activate(4, NodeKind::SingingNotes);
    }

    void ItemSelectorDialog::restoreAnchorPath() {
        auto *sequence = selectionModel()->anchorNodeSelectionModel()->anchorNodeSequenceWithSelectedItems();
        auto *parameter = sequence ? sequence->parameter() : nullptr;
        auto *parameterMap = parameter ? parameter->parameterMap() : nullptr;
        auto *clip = parameterMap ? parameterMap->singingClip() : nullptr;
        auto *track = clip && clip->clipSequence() ? clip->clipSequence()->track() : nullptr;
        if (!track || !parameter || !activate(0, NodeKind::RootTracks) || !activate(1, NodeKind::Track, track) || !activate(2, NodeKind::TrackClips) || !activate(3, NodeKind::Clip, clip) || !activate(4, NodeKind::SingingParameters) || !activate(5, NodeKind::Parameter, parameter)) {
            return;
        }
        activate(6, sequence->role() == dspx::AnchorNodeSequence::Edited ? NodeKind::EditedAnchors : NodeKind::TransformAnchors);
    }

    void ItemSelectorDialog::restoreDynamicAnchorPath() {
        auto *sequence = selectionModel()->dynamicMixingAnchorSelectionModel()->dynamicMixingAnchorSequenceWithSelectedItems();
        auto *sources = sequence ? sequence->sources() : nullptr;
        auto *clip = sources ? sources->singingClip() : nullptr;
        auto *track = clip && clip->clipSequence() ? clip->clipSequence()->track() : nullptr;
        if (!track || !activate(0, NodeKind::RootTracks) || !activate(1, NodeKind::Track, track) || !activate(2, NodeKind::TrackClips) || !activate(3, NodeKind::Clip, clip)) {
            return;
        }
        activate(4, NodeKind::SingingVoiceBlending);
    }

    void ItemSelectorDialog::restoreFreeParameterPath() {
        auto *freeSelection = freeParameterSelectionModel();
        auto *clip = freeSelection->singingClip();
        const auto parameterId = freeSelection->parameterId();
        auto *parameter = clip && !parameterId.isEmpty()
                              ? clip->parameters()->item(parameterId)
                              : nullptr;
        auto *track = clip && clip->clipSequence()
                          ? clip->clipSequence()->track()
                          : nullptr;
        if (!track || !parameter
            || !activate(0, NodeKind::RootTracks)
            || !activate(1, NodeKind::Track, track)
            || !activate(2, NodeKind::TrackClips)
            || !activate(3, NodeKind::Clip, clip)
            || !activate(4, NodeKind::SingingParameters)
            || !activate(5, NodeKind::Parameter, parameter, parameterId)) {
            return;
        }
        activate(
            6,
            freeSelection->layer() == FreeParameterSelectionModel::TransformLayer
                ? NodeKind::FreeformTransform
                : NodeKind::FreeformEdited);
    }

    ItemSelectorDialog::ItemSelectorDialog(ProjectWindowInterface *windowInterface, QWidget *parent)
        : QDialog(parent), m_windowInterface(windowInterface) {
        initialize();
    }

    ItemSelectorDialog::~ItemSelectorDialog() {
        releaseColumns();
    }

    void ItemSelectorDialog::hideEvent(QHideEvent *event) {
        releaseColumns();
        QDialog::hideEvent(event);
    }

    void ItemSelectorDialog::showEvent(QShowEvent *event) {
        QDialog::showEvent(event);
        prepareToShow();
    }

}

#include "ItemSelectorDialog.moc"
