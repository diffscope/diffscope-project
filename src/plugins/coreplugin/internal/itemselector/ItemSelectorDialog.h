// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#ifndef DIFFSCOPE_COREPLUGIN_ITEMSELECTORDIALOG_H
#define DIFFSCOPE_COREPLUGIN_ITEMSELECTORDIALOG_H

#include <QDialog>
#include <QList>
#include <QString>

class QCheckBox;
class QGroupBox;
class QHideEvent;
class QHBoxLayout;
class QModelIndex;
class QScrollArea;
class QShowEvent;
class QTreeView;

namespace Core {
    class ProjectWindowInterface;
}

namespace dspx {
    class Model;
    class SelectionModel;
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
            QGroupBox *widget;
            QCheckBox *selectAll;
            QTreeView *view;
            ItemSelectorListModel *model;
        };

        void initialize();
        void prepareToShow();
        void releaseColumns();
        dspx::Model *model() const;
        dspx::SelectionModel *selectionModel() const;
        void addColumn(ListKind kind, QObject *context);
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

        ProjectWindowInterface *m_windowInterface;
        QScrollArea *m_scrollArea = nullptr;
        QWidget *m_columnsWidget = nullptr;
        QHBoxLayout *m_columnsLayout = nullptr;
        QList<Column> m_columns;
        bool m_mutatingColumns = false;
        int m_horizontalScrollRestoreGeneration = 0;
    };

}

#endif // DIFFSCOPE_COREPLUGIN_ITEMSELECTORDIALOG_H
