// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "FileSystemPermissionDialog.h"

#include <QAction>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QPushButton>
#include <QToolButton>
#include <QVBoxLayout>

namespace BatchProcess::Internal {

    namespace {

        QString accessText(FileAccessMode access) {
            switch (access) {
                case FileAccessMode::Read:
                    return FileSystemPermissionDialog::tr("Read");
                case FileAccessMode::Write:
                    return FileSystemPermissionDialog::tr("Write");
                case FileAccessMode::ReadWrite:
                    return FileSystemPermissionDialog::tr("Read and Write");
            }
            return {};
        }

    }

    FileSystemPermissionDialog::FileSystemPermissionDialog(const QString &scriptName, const QString &scriptId, const QString &pathPattern, FileAccessMode access, const QString &reason, QWidget *parent)
        : QDialog(parent) {
        setWindowTitle(tr("File System Access"));
        setWindowModality(Qt::ApplicationModal);
        setMinimumWidth(560);

        auto titleLabel = new QLabel(tr("Allow this script to access the file system?"), this);
        auto titleFont = titleLabel->font();
        titleFont.setBold(true);
        titleFont.setPointSizeF(titleFont.pointSizeF() * 1.2);
        titleLabel->setFont(titleFont);

        auto formLayout = new QFormLayout;
        formLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
        formLayout->addRow(tr("Script"), new QLabel(QStringLiteral("%1 (%2)").arg(scriptName, scriptId), this));
        formLayout->addRow(tr("Access"), new QLabel(accessText(access), this));

        auto pathEdit = new QLineEdit(pathPattern, this);
        pathEdit->setReadOnly(true);
        pathEdit->setToolTip(pathPattern);
        formLayout->addRow(tr("Path Pattern"), pathEdit);

        auto reasonLabel = new QLabel(reason, this);
        reasonLabel->setWordWrap(true);
        reasonLabel->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
        formLayout->addRow(tr("Reason"), reasonLabel);

        auto allowOnceAction = new QAction(tr("Allow Once"), this);
        auto alwaysAllowAction = new QAction(tr("Always Allow"), this);
        auto allowFullAccessAction = new QAction(tr("Allow Full Access"), this);
        connect(allowOnceAction, &QAction::triggered, this, [this] {
            finish(AllowOnce);
        });
        connect(alwaysAllowAction, &QAction::triggered, this, [this] {
            finish(AlwaysAllow);
        });
        connect(allowFullAccessAction, &QAction::triggered, this, [this] {
            finish(AllowFullAccess);
        });

        auto allowMenu = new QMenu(this);
        allowMenu->addAction(allowOnceAction);
        allowMenu->addAction(alwaysAllowAction);
        allowMenu->addAction(allowFullAccessAction);

        auto allowButton = new QToolButton(this);
        allowButton->setDefaultAction(allowOnceAction);
        allowButton->setMenu(allowMenu);
        allowButton->setPopupMode(QToolButton::MenuButtonPopup);
        allowButton->setToolButtonStyle(Qt::ToolButtonTextOnly);

        auto denyButton = new QPushButton(tr("Deny"), this);
        denyButton->setDefault(true);
        connect(denyButton, &QPushButton::clicked, this, [this] {
            finish(Deny);
        });

        auto buttonBox = new QDialogButtonBox(this);
        buttonBox->addButton(allowButton, QDialogButtonBox::AcceptRole);
        buttonBox->addButton(denyButton, QDialogButtonBox::RejectRole);

        auto layout = new QVBoxLayout(this);
        layout->addWidget(titleLabel);
        layout->addLayout(formLayout);
        layout->addWidget(buttonBox);
    }

    FileSystemPermissionDialog::~FileSystemPermissionDialog() = default;

    FileSystemPermissionDialog::Decision FileSystemPermissionDialog::decision() const {
        return m_decision;
    }

    void FileSystemPermissionDialog::finish(Decision decision) {
        m_decision = decision;
        if (decision == Deny) {
            reject();
        } else {
            accept();
        }
    }

}

#include "moc_FileSystemPermissionDialog.cpp"
