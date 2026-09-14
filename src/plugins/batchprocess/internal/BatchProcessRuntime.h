// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIFFSCOPE_BATCHPROCESS_BATCHPROCESSRUNTIME_H
#define DIFFSCOPE_BATCHPROCESS_BATCHPROCESSRUNTIME_H

#include <memory>

#include <QAbstractListModel>
#include <QHash>
#include <QJSValue>
#include <QList>
#include <QPointer>
#include <QSet>
#include <QString>
#include <QVector>

#include <batchprocess/BatchProcessInterface.h>

class QJSEngine;

namespace Core {
    class ActionWindowInterfaceBase;
}

namespace BatchProcess {

    class BatchProcessInterfacePrivate;
    class Script;
    class ScriptAction;
    class ScriptExecutionContext;

    namespace Internal {

        class RequireBridge;
        class BuiltinRequireBridge;
        class DefineScriptBridge;

        class ScriptActionModel : public QAbstractListModel {
        public:
            enum Role {
                NameRole = Qt::UserRole + 1,
                DescriptionRole,
                ActionRole,
                SeparatorRole,
                RequiresProjectRole,
            };

            explicit ScriptActionModel(QObject *parent = nullptr);

            int rowCount(const QModelIndex &parent = {}) const override;
            QVariant data(const QModelIndex &index, int role) const override;
            QHash<int, QByteArray> roleNames() const override;

            void setScripts(const QList<Script *> &scripts);

        private:
            QList<ScriptAction *> m_actions;
        };

        class BatchProcessRuntime {
        public:
            explicit BatchProcessRuntime(BatchProcessInterfacePrivate *interfacePrivate);
            ~BatchProcessRuntime();

            bool initialize(const QString &scriptDirectory);
            bool executeAction(ScriptAction *action, Core::ActionWindowInterfaceBase *windowInterface);
            void removeOwner(QObject *owner);

            QList<Script *> scripts() const;
            Script *currentScript() const;
            ScriptExecutionContext *executionContext() const;

        private:
            struct BuiltinScriptRegistration;
            struct ModuleRegistration;
            struct GlobalObjectRegistration;
            struct EngineExtensionRegistration;
            struct ActionCallbackRegistration;
            struct DefinitionRecord;
            struct ModuleRecord;
            struct ScriptCandidate;
            struct ErrorInfo;

            friend class RequireBridge;
            friend class BuiltinRequireBridge;
            friend class DefineScriptBridge;

            QJSValue require(Script *script, const QString &rootPath, const QString &entryPath, const QString &currentPath, const QString &specifier, bool singleFile);
            QJSValue requireBuiltin(const QString &specifier);
            QJSValue defineScript(Script *script, const QJSValue &definition);
            QJSValue loadFileModule(Script *script, const QString &rootPath, const QString &entryPath, const QString &filePath, bool singleFile, ErrorInfo *errorInfo);
            QJSValue loadRegisteredModule(const QString &name, QString *errorMessage);

            bool loadCandidate(const ScriptCandidate &candidate, QSet<QString> *loadedIds);
            bool loadBuiltinScript(const BuiltinScriptRegistration &registration, QSet<QString> *loadedIds);
            bool commitScript(std::unique_ptr<Script> script, const DefinitionRecord &definition, QSet<QString> *loadedIds);
            bool applyEngineExtensions();
            void installBuiltinRequire();
            bool resolveFile(const QString &rootPath, const QString &entryPath, const QString &currentPath, const QString &specifier, bool singleFile, QString *resolvedPath, QString *errorMessage) const;
            void setCurrentScript(Script *script);
            void rejectScript(Script *script, const ErrorInfo &errorInfo);
            void purgeScriptModules(Script *script);
            void reportLoadError(const QString &scriptPath, const ErrorInfo &errorInfo) const;
            void reportRuntimeError(const QString &component, const ErrorInfo &errorInfo) const;
            void reportExecutionError(ScriptAction *action, Core::ActionWindowInterfaceBase *windowInterface, const ErrorInfo &errorInfo) const;
            ErrorInfo errorInfo(const QJSValue &error, const QString &moduleId = {}) const;
            QString errorText(const QJSValue &error, const QString &moduleId = {}) const;
            QString virtualModuleId(const QString &rootPath, const QString &filePath) const;
            void clearExecutionContext();

            BatchProcessInterfacePrivate *m_interfacePrivate;
            std::unique_ptr<QJSEngine> m_engine;
            std::unique_ptr<ScriptExecutionContext> m_context;
            QList<Script *> m_scripts;
            QVector<BuiltinScriptRegistration> m_builtinScriptRegistrations;
            QVector<ModuleRegistration> m_moduleRegistrations;
            QVector<GlobalObjectRegistration> m_globalObjectRegistrations;
            QVector<EngineExtensionRegistration> m_engineExtensionRegistrations;
            QVector<ActionCallbackRegistration> m_actionStartedCallbacks;
            QVector<ActionCallbackRegistration> m_actionFinishedCallbacks;
            QHash<QString, QJSValue> m_registeredModules;
            QHash<Script *, QHash<QString, std::shared_ptr<ModuleRecord>>> m_fileModules;
            QHash<Script *, QVector<DefinitionRecord>> m_definitions;
            QJSValue m_runtimeHelpers;
            bool m_executing{};
        };

    }

}

#endif // DIFFSCOPE_BATCHPROCESS_BATCHPROCESSRUNTIME_H
