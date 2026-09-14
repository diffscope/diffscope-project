// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_BATCHPROCESS_FILESYSTEMPERMISSIONDIALOG_H
#define DIFFSCOPE_BATCHPROCESS_FILESYSTEMPERMISSIONDIALOG_H

#include <QDialog>

#include <batchprocess/FileSystemTypes.h>

namespace BatchProcess::Internal {

    class FileSystemPermissionDialog : public QDialog {
        Q_OBJECT
    public:
        enum Decision {
            Deny,
            AllowOnce,
            AlwaysAllow,
            AllowFullAccess,
        };

        FileSystemPermissionDialog(const QString &scriptName, const QString &scriptId, const QString &pathPattern, FileAccessMode access, const QString &reason, QWidget *parent = nullptr);
        ~FileSystemPermissionDialog() override;

        Decision decision() const;

    private:
        void finish(Decision decision);

        Decision m_decision{Deny};
    };

}

#endif // DIFFSCOPE_BATCHPROCESS_FILESYSTEMPERMISSIONDIALOG_H
