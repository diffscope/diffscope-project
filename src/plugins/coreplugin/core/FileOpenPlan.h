// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: Apache-2.0

#ifndef DIFFSCOPE_COREPLUGIN_FILEOPENPLAN_H
#define DIFFSCOPE_COREPLUGIN_FILEOPENPLAN_H

#include <QObject>
#include <QScopedPointer>
#include <QStringList>
#include <qqmlintegration.h>

#include <coreplugin/coreglobal.h>

namespace Core {

    class FileOpenDispatcher;
    class FileOpenDispatcherPrivate;
    class FileOpenPlanPrivate;

    class CORE_EXPORT FileOpenPlan : public QObject {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_DECLARE_PRIVATE(FileOpenPlan)
        Q_PROPERTY(bool acceptsDrop READ acceptsDrop CONSTANT)
        Q_PROPERTY(bool valid READ isValid NOTIFY validChanged)
    public:
        ~FileOpenPlan() override;

        // Returns whether at least one file was accepted during preflight.
        bool acceptsDrop() const;
        // Returns whether this unsubmitted plan retains its handlers and target window.
        bool isValid() const;
        // Returns the absolute file paths in their original order.
        QStringList filePaths() const;

        // Queues this plan once. Returns false if invalid or no file was accepted.
        Q_INVOKABLE bool submit();
        // Invalidates and releases this plan. Submitted requests remain queued.
        Q_INVOKABLE void discard();

    Q_SIGNALS:
        void validChanged();

    private:
        explicit FileOpenPlan(FileOpenPlanPrivate &d, QObject *parent = nullptr);
        void invalidate();

        QScopedPointer<FileOpenPlanPrivate> d_ptr;

        friend class FileOpenDispatcher;
        friend class FileOpenDispatcherPrivate;
    };

}

#endif // DIFFSCOPE_COREPLUGIN_FILEOPENPLAN_H
