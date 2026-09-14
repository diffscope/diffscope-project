// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_BATCHPROCESS_JAVASCRIPTDEBUGCONSOLEDIALOG_H
#define DIFFSCOPE_BATCHPROCESS_JAVASCRIPTDEBUGCONSOLEDIALOG_H

#include <QDialog>

class QAbstractItemModel;
class QListView;
class QShowEvent;
class QSortFilterProxyModel;

namespace BatchProcess::Internal {

    class JavaScriptDebugConsoleDialog : public QDialog {
        Q_OBJECT
    public:
        explicit JavaScriptDebugConsoleDialog(QAbstractItemModel *model, QWidget *parent = nullptr);
        ~JavaScriptDebugConsoleDialog() override;

    protected:
        void showEvent(QShowEvent *event) override;

    private:
        void processViewUpdates();

        QSortFilterProxyModel *m_filterModel{};
        QListView *m_outputView{};
        bool m_followOutput{true};
    };

}

#endif // DIFFSCOPE_BATCHPROCESS_JAVASCRIPTDEBUGCONSOLEDIALOG_H
