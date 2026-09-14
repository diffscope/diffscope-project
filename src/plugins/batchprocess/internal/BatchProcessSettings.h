// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_BATCHPROCESS_BATCHPROCESSSETTINGS_H
#define DIFFSCOPE_BATCHPROCESS_BATCHPROCESSSETTINGS_H

#include <QObject>
#include <QString>

namespace BatchProcess::Internal {

    class BatchProcessPlugin;

    class BatchProcessSettings : public QObject {
        Q_OBJECT
    public:
        static constexpr int DefaultMaximumConsoleMessageCount = 4096;
        static constexpr int MinimumConsoleMessageCount = 1;
        static constexpr int MaximumConsoleMessageCount = 1048576;

        ~BatchProcessSettings() override;

        static BatchProcessSettings *instance();
        static QString scriptDirectory();
        static void setScriptDirectory(const QString &directory);
        static QString defaultScriptDirectory();
        static int maximumConsoleMessageCount();
        static void setMaximumConsoleMessageCount(int maximumMessageCount);

        void load();
        void save() const;

    private:
        friend class BatchProcessPlugin;
        explicit BatchProcessSettings(QObject *parent = nullptr);

        QString m_customScriptDirectory;
        int m_maximumConsoleMessageCount{DefaultMaximumConsoleMessageCount};
    };

}

#endif // DIFFSCOPE_BATCHPROCESS_BATCHPROCESSSETTINGS_H
