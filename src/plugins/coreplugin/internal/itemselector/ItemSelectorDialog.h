// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#ifndef DIFFSCOPE_COREPLUGIN_ITEMSELECTORDIALOG_H
#define DIFFSCOPE_COREPLUGIN_ITEMSELECTORDIALOG_H

#include <QDialog>

class QHideEvent;
class QShowEvent;

namespace Core {
    class ProjectWindowInterface;
}

namespace Core::Internal {

    class ItemSelectorDialogPrivate;

    class ItemSelectorDialog : public QDialog {
        Q_OBJECT

    public:
        explicit ItemSelectorDialog(ProjectWindowInterface *windowInterface, QWidget *parent = nullptr);
        ~ItemSelectorDialog() override;

    protected:
        void hideEvent(QHideEvent *event) override;
        void showEvent(QShowEvent *event) override;

    private:
        ItemSelectorDialogPrivate *d;
    };

}

#endif // DIFFSCOPE_COREPLUGIN_ITEMSELECTORDIALOG_H
