// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#include "ItemSelectorDialog.h"

#include <algorithm>
#include <limits>

#include <QAbstractItemModel>
#include <QAbstractItemView>
#include <QCheckBox>
#include <QFrame>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QHideEvent>
#include <QItemSelectionModel>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPointer>
#include <QScrollArea>
#include <QScrollBar>
#include <QSignalBlocker>
#include <QShowEvent>
#include <QSizePolicy>
#include <QStyle>
#include <QStyleOptionViewItem>
#include <QTimer>
#include <QTreeView>
#include <QVBoxLayout>

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
#include <coreplugin/ProjectDocumentContext.h>
#include <coreplugin/ProjectWindowInterface.h>
#include <coreplugin/internal/ItemSelectorListModel.h>

namespace Core::Internal {

    namespace {

        struct ColumnText {
            QString title;
            QString listAccessibleName;
            QString selectAllAccessibleDescription;
        };

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
                index.data(Qt::CheckStateRole).toInt());
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

        protected:
            void mousePressEvent(QMouseEvent *event) override {
                const auto index = indexAt(event->position().toPoint());
                if (event->button() == Qt::LeftButton && index.isValid()
                    && index.flags().testFlag(Qt::ItemIsUserCheckable)
                    && checkIndicatorRect(index).contains(event->position().toPoint())) {
                    toggleCheckState(index);
                    m_checkboxPressed = true;
                    event->accept();
                    return;
                }
                QTreeView::mousePressEvent(event);
            }

            void mouseReleaseEvent(QMouseEvent *event) override {
                if (m_checkboxPressed) {
                    m_checkboxPressed = false;
                    event->accept();
                    return;
                }
                QTreeView::mouseReleaseEvent(event);
            }

            void mouseDoubleClickEvent(QMouseEvent *event) override {
                const auto index = indexAt(event->position().toPoint());
                if (event->button() == Qt::LeftButton && index.isValid()
                    && index.flags().testFlag(Qt::ItemIsUserCheckable)
                    && checkIndicatorRect(index).contains(event->position().toPoint())) {
                    m_checkboxPressed = true;
                    event->accept();
                    return;
                }
                QTreeView::mouseDoubleClickEvent(event);
            }

            void keyPressEvent(QKeyEvent *event) override {
                if (event->key() == Qt::Key_Space && currentIndex().isValid()
                    && currentIndex().flags().testFlag(Qt::ItemIsUserCheckable)) {
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
                    index.data(Qt::CheckStateRole).toInt());
                return style()->subElementRect(
                    QStyle::SE_ItemViewItemCheckIndicator, &option, this);
            }

            bool m_checkboxPressed = false;
        };

        class SelectAllCheckBox : public QCheckBox {
        public:
            explicit SelectAllCheckBox(QWidget *parent = nullptr) : QCheckBox(parent) {
                setTristate(true);
            }

        protected:
            void nextCheckState() override {
                setCheckState(checkState() == Qt::Checked
                                  ? Qt::Unchecked
                                  : Qt::Checked);
            }
        };

    }

    void ItemSelectorDialog::initialize() {
            setWindowTitle(ItemSelectorDialog::tr("Item Selector"));
            setWindowModality(Qt::NonModal);
            setWindowFlag(Qt::Tool);
            resize(960, 620);

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

    void ItemSelectorDialog::addColumn(ListKind kind, QObject *context) {
            const auto text = columnText(kind, context);
            auto *columnWidget = new QGroupBox(text.title, m_columnsWidget);
            columnWidget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
            auto *layout = new QVBoxLayout(columnWidget);
            layout->setContentsMargins(0, 0, 0, 0);
            layout->setSpacing(4);

            auto *model = new ItemSelectorListModel(kind, context, m_windowInterface, columnWidget);
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
            m_columns.append({columnWidget, selectAll, view, model});
            updateColumnWidth(columnWidget);

            const auto updateSelectAll = [model, selectAll] {
                const QSignalBlocker blocker(selectAll);
                selectAll->setEnabled(model->hasSelectableItems());
                selectAll->setCheckState(model->checkStateSummary());
            };
            updateSelectAll();
            QObject::connect(model, &ItemSelectorListModel::checkStateSummaryChanged,
                             this, updateSelectAll);
            QObject::connect(model, &QAbstractItemModel::rowsInserted, this,
                             [this, columnWidget, updateSelectAll] {
                                 updateSelectAll();
                                 updateColumnWidth(columnWidget);
                             });
            QObject::connect(model, &QAbstractItemModel::rowsRemoved, this,
                             [this, columnWidget, updateSelectAll] {
                                 updateSelectAll();
                                 updateColumnWidth(columnWidget);
                             });
            QObject::connect(model, &QAbstractItemModel::modelReset, this,
                             [this, columnWidget] {
                                 updateColumnWidth(columnWidget);
                             });
            QObject::connect(model, &QAbstractItemModel::dataChanged, this,
                             [this, columnWidget](const QModelIndex &, const QModelIndex &,
                                                  const QList<int> &roles) {
                                 if (roles.isEmpty() || roles.contains(Qt::DisplayRole)) {
                                     updateColumnWidth(columnWidget);
                                 }
                             });
            QObject::connect(selectAll, &QCheckBox::clicked, this, [model, selectAll] {
                model->setAllSelected(selectAll->checkState() == Qt::Checked);
            });

            QObject::connect(view->selectionModel(), &QItemSelectionModel::currentChanged,
                             this, [this, columnWidget](const QModelIndex &current) {
                                 if (m_mutatingColumns) {
                                     return;
                                 }
                                 const int columnIndex = indexOfColumn(columnWidget);
                                 if (columnIndex >= 0) {
                                     navigateFrom(columnIndex, current);
                                 }
                             });
            QObject::connect(view, &QAbstractItemView::activated, this,
                             [this, columnWidget, model](const QModelIndex &index) {
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

                                 QObject *item = entry.object.data();
                                 auto *selection = selectionModel();
                                 if (!supportsDocumentCurrentItem(entry.kind) || !item
                                     || selection->currentItem() == item) {
                                     return;
                                 }
                                 selection->select(item, dspx::SelectionModel::SetCurrentItem);
                             });
            QObject::connect(model, &QAbstractItemModel::rowsAboutToBeRemoved,
                             this, [this, columnWidget, view](const QModelIndex &, int first, int last) {
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
            QObject::connect(model, &QAbstractItemModel::modelAboutToBeReset,
                             this, [this, columnWidget, view] {
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
            QObject::connect(view, &ItemSelectorView::focusPreviousRequested, this,
                             [this, columnWidget] {
                                 const int index = indexOfColumn(columnWidget);
                                 if (index > 0) {
                                     focusColumn(index - 1, true);
                                 }
                             });
            QObject::connect(view, &ItemSelectorView::focusNextRequested, this,
                             [this, columnWidget] {
                                 const int index = indexOfColumn(columnWidget);
                                 if (index >= 0 && index + 1 < m_columns.size()) {
                                     focusColumn(index + 1, true);
                                 }
                             });
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
            const QPointer<QCheckBox> selectAll = m_columns.at(columnIndex).selectAll;
            const QPointer<QTreeView> view = m_columns.at(columnIndex).view;
            const QPointer<ItemSelectorListModel> model = m_columns.at(columnIndex).model;
            if (!column || !selectAll || !view || !model) {
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
                    view->sizeHintForIndex(model->index(row)).width());
            }

            const auto layoutMargins = column->layout()->contentsMargins();
            const auto groupMargins = column->contentsMargins();
            const int horizontalLayoutMargins = layoutMargins.left() + layoutMargins.right()
                                                + groupMargins.left() + groupMargins.right();
            const int contentPreferredWidth = contentWidth + view->frameWidth() * 2
                                              + horizontalLayoutMargins + 12;
            const int selectAllPreferredWidth = selectAll->isVisible()
                                                    ? selectAll->sizeHint().width()
                                                          + horizontalLayoutMargins
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
                std::clamp(std::max(contentPreferredWidth, selectAllPreferredWidth),
                           minimumColumnWidth, maximumColumnWidth));
            applyColumnWidth(targetWidth);
            if (view->verticalScrollBar()->maximum()
                > view->verticalScrollBar()->minimum()) {
                const int scrollBarExtent = view->style()->pixelMetric(
                    QStyle::PM_ScrollBarExtent, nullptr, view);
                targetWidth = std::max(
                    groupTitlePreferredWidth,
                    std::clamp(std::max(contentPreferredWidth + scrollBarExtent,
                                        selectAllPreferredWidth),
                               minimumColumnWidth, maximumColumnWidth));
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
                std::min(horizontalScrollValue, view->horizontalScrollBar()->maximum()));
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
            const QPointer<QTreeView> view = m_columns.constLast().view;
            QTimer::singleShot(0, this, [this, column, view] {
                if (!column || !view || !isVisible()) {
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
                view->setFocus();
                m_scrollArea->ensureWidgetVisible(column, m_columnsLayout->spacing(), 0);
            });
        }

    void ItemSelectorDialog::focusColumn(int columnIndex, bool ensureVisible) {
            if (columnIndex < 0 || columnIndex >= m_columns.size()) {
                return;
            }
            const QPointer<QWidget> column = m_columns.at(columnIndex).widget;
            const QPointer<QTreeView> view = m_columns.at(columnIndex).view;
            if (!column || !view) {
                return;
            }
            view->setFocus();
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
                scrollBar->setValue(std::clamp(value,
                                               scrollBar->minimum(), scrollBar->maximum()));
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
            ListKind nextKind = ListKind::Root;
            QObject *nextContext = nullptr;

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
                    nextKind = ListKind::Clips;
                    nextContext = entry.object.data();
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
                    break;
                case NodeKind::EditedAnchors: {
                    auto *parameter = qobject_cast<dspx::Parameter *>(columnModel->context());
                    hasNextColumn = true;
                    nextKind = ListKind::Anchors;
                    nextContext = parameter ? parameter->anchorEdited() : nullptr;
                    break;
                }
                case NodeKind::TransformAnchors: {
                    auto *parameter = qobject_cast<dspx::Parameter *>(columnModel->context());
                    hasNextColumn = true;
                    nextKind = ListKind::Anchors;
                    nextContext = parameter ? parameter->anchorTransform() : nullptr;
                    break;
                }
                default:
                    break;
                }
            }

            const int nextColumnIndex = columnIndex + 1;
            if (hasNextColumn && nextColumnIndex < m_columns.size()) {
                const QPointer<ItemSelectorListModel> nextColumnModel = m_columns.at(nextColumnIndex).model;
                if (nextColumnModel && nextColumnModel->listKind() == nextKind
                    && nextColumnModel->context() == nextContext) {
                    return;
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
                addColumn(nextKind, nextContext);
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

    bool ItemSelectorDialog::activate(int columnIndex, NodeKind kind, QObject *object,
                                      const QString &key) {
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
                std::min(horizontalScrollValue,
                         columnView->horizontalScrollBar()->maximum()));
            return true;
        }

    void ItemSelectorDialog::restoreInitialPath() {
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
            activate(1, NodeKind::Track, targetTrack);
        }

    void ItemSelectorDialog::restoreNotePath() {
            auto *sequence = selectionModel()->noteSelectionModel()->noteSequenceWithSelectedItems();
            auto *clip = sequence ? sequence->singingClip() : nullptr;
            auto *track = clip && clip->clipSequence() ? clip->clipSequence()->track() : nullptr;
            if (!track || !activate(0, NodeKind::RootTracks)
                || !activate(1, NodeKind::Track, track)
                || !activate(2, NodeKind::Clip, clip)) {
                return;
            }
            activate(3, NodeKind::SingingNotes);
        }

    void ItemSelectorDialog::restoreAnchorPath() {
            auto *sequence = selectionModel()->anchorNodeSelectionModel()->anchorNodeSequenceWithSelectedItems();
            auto *parameter = sequence ? sequence->parameter() : nullptr;
            auto *parameterMap = parameter ? parameter->parameterMap() : nullptr;
            auto *clip = parameterMap ? parameterMap->singingClip() : nullptr;
            auto *track = clip && clip->clipSequence() ? clip->clipSequence()->track() : nullptr;
            if (!track || !parameter || !activate(0, NodeKind::RootTracks)
                || !activate(1, NodeKind::Track, track)
                || !activate(2, NodeKind::Clip, clip)
                || !activate(3, NodeKind::SingingParameters)
                || !activate(4, NodeKind::Parameter, parameter)) {
                return;
            }
            activate(5, sequence->role() == dspx::AnchorNodeSequence::Edited
                            ? NodeKind::EditedAnchors
                            : NodeKind::TransformAnchors);
        }

    void ItemSelectorDialog::restoreDynamicAnchorPath() {
            auto *sequence = selectionModel()->dynamicMixingAnchorSelectionModel()
                                 ->dynamicMixingAnchorSequenceWithSelectedItems();
            auto *sources = sequence ? sequence->sources() : nullptr;
            auto *clip = sources ? sources->singingClip() : nullptr;
            auto *track = clip && clip->clipSequence() ? clip->clipSequence()->track() : nullptr;
            if (!track || !activate(0, NodeKind::RootTracks)
                || !activate(1, NodeKind::Track, track)
                || !activate(2, NodeKind::Clip, clip)) {
                return;
            }
            activate(3, NodeKind::SingingVoiceBlending);
        }

    ItemSelectorDialog::ItemSelectorDialog(ProjectWindowInterface *windowInterface,
                                           QWidget *parent)
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
