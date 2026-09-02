// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#ifndef DIFFSCOPE_COREPLUGIN_ITEMSELECTORDIALOG_H
#define DIFFSCOPE_COREPLUGIN_ITEMSELECTORDIALOG_H

#include <QDialog>
#include <QList>
#include <QPointer>
#include <QString>

#include <dspxmodelSelectionModel/SelectionModel.h>

class QCheckBox;
class QGroupBox;
class QHideEvent;
class QHBoxLayout;
class QModelIndex;
class QScrollArea;
class QShowEvent;
class QTreeView;

namespace Core {
    class FreeParameterSelectionModel;
    class ProjectWindowInterface;
}

namespace dspx {
    class Model;
    class SingingClip;
}

namespace Core::Internal {

    class ItemSelectorListModel;
    enum class ListKind;
    enum class NodeKind;

    class ItemSelectorDialog : public QDialog {
        Q_OBJECT

    public:
        explicit ItemSelectorDialog(ProjectWindowInterface *windowInterface, QWidget *parent = nullptr);
        ~ItemSelectorDialog() override;

    protected:
        void hideEvent(QHideEvent *event) override;
        void showEvent(QShowEvent *event) override;

    private:
        struct Column {
            QGroupBox *widget = nullptr;
            QCheckBox *selectAll = nullptr;
            QWidget *focusWidget = nullptr;
            QTreeView *view = nullptr;
            ItemSelectorListModel *model = nullptr;
            bool rangeColumn = false;
            QPointer<QObject> context;
            QString contextKey;
            bool transform = false;
        };

        void initialize();
        void prepareToShow();
        void releaseColumns();
        dspx::Model *model() const;
        dspx::SelectionModel *selectionModel() const;
        FreeParameterSelectionModel *freeParameterSelectionModel() const;
        void popupDocumentContextMenu(dspx::SelectionModel::SelectionType selectionType,
                                      bool sceneContextMenu);
        void addColumn(ListKind kind, QObject *context, const QString &contextKey = {});
        void addRangeColumn(dspx::SingingClip *clip,
                            ItemSelectorListModel *parameterComponentsModel,
                            bool transform);
        void updateColumnWidth(QWidget *columnWidget);
        void focusInitialColumn();
        void focusColumn(int columnIndex, bool ensureVisible);
        void restoreHorizontalScrollPosition(int value);
        void removeColumnsAfterPreservingScroll(int columnIndex);
        void navigateFrom(int columnIndex, const QModelIndex &current);
        void removeColumnsAfter(int columnIndex);
        int indexOfColumn(QWidget *widget) const;
        bool activate(int columnIndex, NodeKind kind, QObject *object = nullptr,
                      const QString &key = {});
        void restoreInitialPath();
        void restoreClipPath();
        void restoreNotePath();
        void restoreAnchorPath();
        void restoreDynamicAnchorPath();
        void restoreFreeParameterPath();

        ProjectWindowInterface *m_windowInterface;
        QScrollArea *m_scrollArea = nullptr;
        QWidget *m_columnsWidget = nullptr;
        QHBoxLayout *m_columnsLayout = nullptr;
        QList<Column> m_columns;
        bool m_mutatingColumns = false;
        int m_horizontalScrollRestoreGeneration = 0;
        QObject *m_menuLayerObject = nullptr;
    };

}

#endif // DIFFSCOPE_COREPLUGIN_ITEMSELECTORDIALOG_H
