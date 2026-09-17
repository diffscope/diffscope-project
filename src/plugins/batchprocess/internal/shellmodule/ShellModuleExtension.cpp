// SPDX-FileCopyrightText: Team OpenVPI
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "ShellModuleExtension.h"

#include <algorithm>
#include <memory>

#include <QAbstractButton>
#include <QColor>
#include <QCoreApplication>
#include <QDialogButtonBox>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QJSEngine>
#include <QJSValue>
#include <QLayout>
#include <QLoggingCategory>
#include <QMetaObject>
#include <QPointer>
#include <QQmlComponent>
#include <QPushButton>
#include <QQuickItem>
#include <QQuickWindow>
#include <QSet>
#include <QStandardItemModel>
#include <QStringList>
#include <QUrl>
#include <QVariant>
#include <QWindow>

#include <CoreApi/runtimeinterface.h>
#include <CoreApi/windowsystem.h>

#include <SVSCraftCore/SVSCraftNamespace.h>
#include <SVSCraftQuick/MessageBox.h>

#include <coreplugin/ActionWindowInterfaceBase.h>
#include <coreplugin/CoreInterface.h>
#include <coreplugin/ProjectWindowInterface.h>
#include <coreplugin/QuickInput.h>
#include <coreplugin/QuickPick.h>

#include <batchprocess/BatchProcessInterface.h>
#include <batchprocess/FileSystemAccessInterface.h>
#include <batchprocess/FileSystemTypes.h>
#include <batchprocess/Script.h>
#include <batchprocess/ScriptExecutionContext.h>
#include <batchprocess/ShellInterface.h>
#include <batchprocess/private/ShellInterface_p.h>

namespace BatchProcess::Internal {

    Q_STATIC_LOGGING_CATEGORY(lcShellModule, "diffscope.batchprocess.shellmodule")

    namespace {

        constexpr auto ModuleName = "diffscope:shell";
        constexpr auto ModuleResourcePath = ":/diffscope/batchprocess/internalscripts/shell/shell-module.js";
        constexpr auto ModuleSourceUrl = "qrc:/diffscope/batchprocess/internalscripts/shell/shell-module.js";

        struct MessageButtonData {
            QString id;
            QString text;
            QString role;
            bool isDefault{};
            bool isEscape{};
        };

        QString javaScriptErrorText(const QJSValue &error) {
            auto text = error.toString();
            const auto stack = error.property(QStringLiteral("stack")).toString();
            if (!stack.isEmpty() && !text.contains(stack)) {
                text.append(QLatin1Char('\n'));
                text.append(stack);
            }
            return text;
        }

        bool isUsableWindow(Core::ActionWindowInterfaceBase *windowInterface) {
            return windowInterface && !windowInterface->isEffectivelyClosed() && windowInterface->window();
        }

        SVS::SVSCraft::MessageBoxIcon messageIcon(const QString &severity) {
            if (severity == QStringLiteral("success")) {
                return SVS::SVSCraft::Success;
            }
            if (severity == QStringLiteral("warning")) {
                return SVS::SVSCraft::Warning;
            }
            if (severity == QStringLiteral("critical")) {
                return SVS::SVSCraft::Critical;
            }
            if (severity == QStringLiteral("question")) {
                return SVS::SVSCraft::Question;
            }
            return SVS::SVSCraft::Information;
        }

        QDialogButtonBox::ButtonRole buttonRole(const QString &role) {
            if (role == QStringLiteral("accept")) {
                return QDialogButtonBox::AcceptRole;
            }
            if (role == QStringLiteral("reject")) {
                return QDialogButtonBox::RejectRole;
            }
            if (role == QStringLiteral("destructive")) {
                return QDialogButtonBox::DestructiveRole;
            }
            if (role == QStringLiteral("help")) {
                return QDialogButtonBox::HelpRole;
            }
            return QDialogButtonBox::ActionRole;
        }

        QList<MessageButtonData> platformOrderedButtons(const QList<MessageButtonData> &buttons) {
            QDialogButtonBox buttonBox;
            QHash<QAbstractButton *, qsizetype> sourceIndexes;
            for (qsizetype index = 0; index < buttons.size(); ++index) {
                auto button = buttonBox.addButton(buttons.at(index).text, buttonRole(buttons.at(index).role));
                sourceIndexes.insert(button, index);
            }
            buttonBox.ensurePolished();
            buttonBox.layout()->activate();

            QList<MessageButtonData> result;
            result.reserve(buttons.size());
            QSet<qsizetype> appended;
            for (int index = 0; index < buttonBox.layout()->count(); ++index) {
                const auto item = buttonBox.layout()->itemAt(index);
                const auto button = item ? qobject_cast<QAbstractButton *>(item->widget()) : nullptr;
                if (!button || !sourceIndexes.contains(button)) {
                    continue;
                }
                const auto sourceIndex = sourceIndexes.value(button);
                result.append(buttons.at(sourceIndex));
                appended.insert(sourceIndex);
            }
            for (qsizetype index = 0; index < buttons.size(); ++index) {
                if (!appended.contains(index)) {
                    result.append(buttons.at(index));
                }
            }
            return result;
        }

        QString fileDialogFilters(const QJSValue &filters) {
            QStringList result;
            const auto length = filters.property(QStringLiteral("length")).toUInt();
            result.reserve(static_cast<qsizetype>(length));
            for (quint32 index = 0; index < length; ++index) {
                const auto filter = filters.property(index);
                const auto patternsValue = filter.property(QStringLiteral("patterns"));
                QStringList patterns;
                const auto patternCount = patternsValue.property(QStringLiteral("length")).toUInt();
                patterns.reserve(static_cast<qsizetype>(patternCount));
                for (quint32 patternIndex = 0; patternIndex < patternCount; ++patternIndex) {
                    patterns.append(patternsValue.property(patternIndex).toString());
                }
                result.append(QStringLiteral("%1 (%2)").arg(filter.property(QStringLiteral("name")).toString(), patterns.join(QLatin1Char(' '))));
            }
            return result.join(QStringLiteral(";;"));
        }

        QString cssColor(const QColor &color) {
            if (color.alpha() == 255) {
                return color.name(QColor::HexRgb);
            }
            return QStringLiteral("rgba(%1, %2, %3, %4)")
                .arg(color.red())
                .arg(color.green())
                .arg(color.blue())
                .arg(QString::number(color.alphaF(), 'g', 8));
        }

        FileAccessMode fileAccessMode(const QString &access) {
            if (access == QStringLiteral("write")) {
                return FileAccessMode::Write;
            }
            if (access == QStringLiteral("readWrite")) {
                return FileAccessMode::ReadWrite;
            }
            return FileAccessMode::Read;
        }

    }

    class ShellModuleBridge;

    class MessageDialogCloseTracker : public QObject {
        Q_OBJECT
    public:
        using QObject::QObject;

        bool isDone() const {
            return m_done;
        }

    public Q_SLOTS:
        void handleDone(const QVariant &) {
            m_done = true;
        }

    private:
        bool m_done{};
    };

    class WindowBridge : public QObject {
        Q_OBJECT
        Q_PROPERTY(bool valid READ isValid)
        Q_PROPERTY(bool active READ isActive)
    public:
        WindowBridge(ShellModuleBridge *moduleBridge, Core::ActionWindowInterfaceBase *windowInterface, QObject *parent);

        bool isValid() const;
        bool isActive() const;
        Core::ActionWindowInterfaceBase *windowInterface() const;
        ScriptExecutionContext *context() const;

        Q_INVOKABLE QJSValue raise() const;

    private:
        QPointer<ShellModuleBridge> m_moduleBridge;
        QPointer<Core::ActionWindowInterfaceBase> m_windowInterface;
    };

    class ShellModuleBridge : public QObject {
        Q_OBJECT
        Q_PROPERTY(QString scriptName READ scriptName)
        Q_PROPERTY(QString okText READ okText CONSTANT)
        Q_PROPERTY(QString cancelText READ cancelText CONSTANT)
    public:
        ShellModuleBridge(ShellModuleExtension *extension, FileSystemAccessInterface *fileSystemAccessInterface, ScriptExecutionContext *context, QJSEngine *engine, QObject *parent)
            : QObject(parent), m_extension(extension), m_fileSystemAccessInterface(fileSystemAccessInterface), m_context(context), m_engine(engine) {
        }

        QString scriptName() const {
            return m_context && m_context->script() ? m_context->script()->metadata().name() : QString();
        }

        QString okText() const {
            return QCoreApplication::translate("QPlatformTheme", "OK");
        }

        QString cancelText() const {
            return QCoreApplication::translate("QPlatformTheme", "Cancel");
        }

        ScriptExecutionContext *context() const {
            return m_context.data();
        }

        QJSEngine *engine() const {
            return m_engine;
        }

        QJSValue success(const QJSValue &value = QJSValue(QJSValue::UndefinedValue)) const {
            auto result = m_engine->newObject();
            result.setProperty(QStringLiteral("ok"), true);
            result.setProperty(QStringLiteral("value"), value);
            return result;
        }

        QJSValue windowFailure(const QString &code, const QString &message) const {
            auto error = m_engine->newObject();
            error.setProperty(QStringLiteral("windowError"), true);
            error.setProperty(QStringLiteral("code"), code);
            error.setProperty(QStringLiteral("message"), message);
            auto result = m_engine->newObject();
            result.setProperty(QStringLiteral("ok"), false);
            result.setProperty(QStringLiteral("error"), error);
            return result;
        }

        QJSValue hostFailure(const QString &message) const {
            auto error = m_engine->newObject();
            error.setProperty(QStringLiteral("windowError"), false);
            error.setProperty(QStringLiteral("message"), message);
            auto result = m_engine->newObject();
            result.setProperty(QStringLiteral("ok"), false);
            result.setProperty(QStringLiteral("error"), error);
            return result;
        }

        bool actionIsExecuting() const {
            return m_context && m_context->engine() == m_engine && m_context->script() && m_context->action();
        }

        Core::ActionWindowInterfaceBase *resolveWindow(const QJSValue &parentValue, QJSValue *error) const {
            if (!actionIsExecuting()) {
                *error = windowFailure(QStringLiteral("unsupportedOperation"), ShellInterface::tr("Shell operations are only available while a script action is executing."));
                return nullptr;
            }
            if (!parentValue.isNull() && !parentValue.isUndefined()) {
                const auto windowBridge = qobject_cast<WindowBridge *>(parentValue.toQObject());
                const auto windowInterface = windowBridge && windowBridge->context() == m_context ? windowBridge->windowInterface() : nullptr;
                if (!isUsableWindow(windowInterface)) {
                    *error = windowFailure(QStringLiteral("invalidWindow"), ShellInterface::tr("The application window handle is no longer valid."));
                    return nullptr;
                }
                return windowInterface;
            }

            const auto windowSystem = Core::CoreInterface::windowSystem();
            if (windowSystem) {
                const auto windows = windowSystem->windows();
                const auto active = std::ranges::find_if(windows, [](Core::WindowInterface *window) {
                    const auto actionWindow = qobject_cast<Core::ActionWindowInterfaceBase *>(window);
                    return isUsableWindow(actionWindow) && actionWindow->window()->isActive();
                });
                if (active != windows.cend()) {
                    return qobject_cast<Core::ActionWindowInterfaceBase *>(*active);
                }
                const auto first = std::ranges::find_if(windows, [](Core::WindowInterface *window) {
                    return isUsableWindow(qobject_cast<Core::ActionWindowInterfaceBase *>(window));
                });
                if (first != windows.cend()) {
                    return qobject_cast<Core::ActionWindowInterfaceBase *>(*first);
                }
            }
            *error = windowFailure(QStringLiteral("unsupportedOperation"), ShellInterface::tr("No application window is available for this operation."));
            return nullptr;
        }

        QJSValue raiseWindow(Core::ActionWindowInterfaceBase *windowInterface) const {
            if (!actionIsExecuting()) {
                return windowFailure(QStringLiteral("unsupportedOperation"), ShellInterface::tr("Shell operations are only available while a script action is executing."));
            }
            if (!isUsableWindow(windowInterface)) {
                return windowFailure(QStringLiteral("invalidWindow"), ShellInterface::tr("The application window handle is no longer valid."));
            }
            auto window = windowInterface->window();
            window->show();
            window->raise();
            window->requestActivate();
            return success();
        }

        Q_INVOKABLE QJSValue message(const QString &title, const QString &message, const QString &detail, const QString &severity, const QJSValue &buttonsValue, const QJSValue &parentValue) const {
            QJSValue error;
            const auto windowInterface = resolveWindow(parentValue, &error);
            if (!windowInterface) {
                return error;
            }

            QList<MessageButtonData> buttons;
            if (buttonsValue.isUndefined()) {
                buttons.append({QStringLiteral("ok"), okText(), QStringLiteral("accept"), true, true});
            } else {
                const auto length = buttonsValue.property(QStringLiteral("length")).toUInt();
                buttons.reserve(static_cast<qsizetype>(length));
                for (quint32 index = 0; index < length; ++index) {
                    const auto button = buttonsValue.property(index);
                    buttons.append({
                        button.property(QStringLiteral("id")).toString(),
                        button.property(QStringLiteral("text")).toString(),
                        button.property(QStringLiteral("role")).toString(),
                        button.property(QStringLiteral("default")).toBool(),
                        button.property(QStringLiteral("escape")).toBool(),
                    });
                }
            }
            const auto orderedButtons = platformOrderedButtons(buttons);
            QVariantList buttonModel;
            QString primaryButton;
            QString escapeButton;
            for (const auto &button : orderedButtons) {
                buttonModel.append(QVariantMap{{QStringLiteral("id"), button.id}, {QStringLiteral("text"), button.text}});
                if (button.isDefault) {
                    primaryButton = button.id;
                }
                if (button.isEscape) {
                    escapeButton = button.id;
                }
            }
            if (primaryButton.isEmpty()) {
                const auto acceptButton = std::ranges::find_if(orderedButtons, [](const MessageButtonData &button) {
                    return button.role == QStringLiteral("accept") || button.role == QStringLiteral("destructive");
                });
                primaryButton = acceptButton == orderedButtons.cend() ? orderedButtons.constFirst().id : acceptButton->id;
            }
            QQmlComponent component(Core::RuntimeInterface::qmlEngine(), QUrl(QStringLiteral("qrc:/qt/qml/SVSCraft/UIComponents/qml/MessageBoxDialog.qml")));
            if (component.isError()) {
                qFatal() << component.errorString();
            }
            std::unique_ptr<QObject> dialog(component.createWithInitialProperties({
                {QStringLiteral("title"), title},
                {QStringLiteral("text"), title},
                {QStringLiteral("informativeText"), message},
                {QStringLiteral("detailedText"), detail},
                {QStringLiteral("textFormat"), Qt::PlainText},
                {QStringLiteral("buttons"), buttonModel},
                {QStringLiteral("primaryButton"), primaryButton},
                {QStringLiteral("escapeButton"), escapeButton},
                {QStringLiteral("icon"), QVariant::fromValue(messageIcon(severity))},
                {QStringLiteral("transientParent"), QVariant::fromValue(windowInterface->window())},
            }));
            if (!dialog) {
                qFatal() << component.errorString();
            }
            if (escapeButton.isEmpty()) {
                const auto quickDialog = qobject_cast<QQuickWindow *>(dialog.get());
                if (!quickDialog) {
                    qFatal("MessageBoxDialog in SVSCraft.UIComponents is not QQuickWindow");
                }
                quickDialog->setFlag(Qt::WindowCloseButtonHint, true);
                MessageDialogCloseTracker closeTracker;
                connect(dialog.get(), SIGNAL(done(QVariant)), &closeTracker, SLOT(handleDone(QVariant)));
                connect(quickDialog, &QWindow::visibleChanged, &closeTracker, [dialog = dialog.get(), &closeTracker](bool visible) {
                    if (!visible && !closeTracker.isDone()) {
                        QMetaObject::invokeMethod(dialog, "done", Q_ARG(QVariant, QVariant()));
                    }
                });
                qCInfo(lcShellModule) << "Showing script message dialog";
                const auto result = SVS::MessageBox::customExec(dialog.get());
                return success(result.isValid() ? QJSValue(result.toString()) : QJSValue(QJSValue::NullValue));
            }
            qCInfo(lcShellModule) << "Showing script message dialog";
            const auto result = SVS::MessageBox::customExec(dialog.get());
            return success(result.isValid() ? QJSValue(result.toString()) : QJSValue(QJSValue::NullValue));
        }

        Q_INVOKABLE QJSValue promptText(const QString &label, const QString &title, bool titleProvided, const QString &initialValue, const QString &placeholder, bool multiline, bool allowEmpty, const QJSValue &parentValue) const {
            QJSValue error;
            const auto windowInterface = resolveWindow(parentValue, &error);
            if (!windowInterface) {
                return error;
            }
            if (!multiline && !titleProvided) {
                Core::QuickInput input;
                input.setWindowHandle(windowInterface);
                input.setPlaceholderText(placeholder);
                input.setText(initialValue);
                bool attempted{};
                const auto updateValidation = [&] {
                    const auto acceptable = allowEmpty || !input.text().trimmed().isEmpty();
                    input.setAcceptable(acceptable);
                    input.setStatus(attempted && !acceptable ? SVS::SVSCraft::CT_Error : SVS::SVSCraft::CT_Normal);
                    input.setPromptText(attempted && !acceptable
                        ? ShellInterface::tr("%1\nA value is required.").arg(label)
                        : label);
                };
                connect(&input, &Core::QuickInput::textChanged, &input, updateValidation);
                connect(&input, &Core::QuickInput::attemptingAcceptButFailed, &input, [&] {
                    attempted = true;
                    updateValidation();
                });
                updateValidation();
                const auto result = input.exec();
                return success(result.isValid() ? QJSValue(result.toString()) : QJSValue(QJSValue::NullValue));
            }

            const auto quickWindow = qobject_cast<QQuickWindow *>(windowInterface->window());
            if (!quickWindow || !quickWindow->contentItem()) {
                return windowFailure(QStringLiteral("unsupportedOperation"), ShellInterface::tr("The application window cannot host this text input dialog."));
            }
            QQmlComponent component(Core::RuntimeInterface::qmlEngine(), QStringLiteral("DiffScope.BatchProcess"), QStringLiteral("ShellTextPromptDialog"));
            if (component.isError()) {
                qFatal() << component.errorString();
            }
            std::unique_ptr<QObject> dialog(component.createWithInitialProperties({
                {QStringLiteral("parent"), QVariant::fromValue(quickWindow->contentItem())},
                {QStringLiteral("title"), titleProvided ? title : scriptName()},
                {QStringLiteral("labelText"), label},
                {QStringLiteral("inputText"), initialValue},
                {QStringLiteral("placeholderText"), placeholder},
                {QStringLiteral("multiline"), multiline},
                {QStringLiteral("allowEmpty"), allowEmpty},
            }));
            if (!dialog) {
                qFatal() << component.errorString();
            }
            positionDialog(dialog.get(), quickWindow);
            const auto accepted = execDialog(dialog.get());
            return success(accepted ? QJSValue(dialog->property("inputText").toString()) : QJSValue(QJSValue::NullValue));
        }

        Q_INVOKABLE QJSValue choose(const QString &title, const QString &placeholder, const QJSValue &itemsValue, const QString &initialValue, const QString &initialFilter, const QJSValue &parentValue) const {
            QJSValue error;
            const auto windowInterface = resolveWindow(parentValue, &error);
            if (!windowInterface) {
                return error;
            }
            QStandardItemModel model;
            int initialIndex = -1;
            int firstEnabledIndex = -1;
            const auto length = itemsValue.property(QStringLiteral("length")).toUInt();
            for (quint32 index = 0; index < length; ++index) {
                const auto itemValue = itemsValue.property(index);
                auto item = new QStandardItem;
                const auto enabled = itemValue.property(QStringLiteral("enabled")).toBool();
                const auto stableValue = itemValue.property(QStringLiteral("value")).toString();
                item->setData(itemValue.property(QStringLiteral("label")).toString(), SVS::SVSCraft::CP_TitleRole);
                item->setData(itemValue.property(QStringLiteral("description")).toString(), SVS::SVSCraft::CP_DescriptionRole);
                item->setData(stableValue, Qt::UserRole);
                if (!enabled) {
                    item->setFlags(Qt::NoItemFlags);
                } else if (firstEnabledIndex < 0) {
                    firstEnabledIndex = static_cast<int>(index);
                }
                if (enabled && stableValue == initialValue) {
                    initialIndex = static_cast<int>(index);
                }
                model.appendRow(item);
            }
            if (firstEnabledIndex < 0) {
                return success(QJSValue(QJSValue::NullValue));
            }
            Core::QuickPick quickPick;
            quickPick.setWindowHandle(windowInterface);
            quickPick.setModel(&model);
            quickPick.setPlaceholderText(placeholder.isEmpty() ? title : QStringLiteral("%1 — %2").arg(title, placeholder));
            quickPick.setFilterText(initialFilter);
            quickPick.setCurrentIndex(initialIndex >= 0 ? initialIndex : firstEnabledIndex);
            const auto index = quickPick.exec();
            if (index < 0 || index >= model.rowCount() || !(model.item(index)->flags() & Qt::ItemIsEnabled)) {
                return success(QJSValue(QJSValue::NullValue));
            }
            return success(QJSValue(model.item(index)->data(Qt::UserRole).toString()));
        }

        Q_INVOKABLE QJSValue form(const QString &title, const QString &description, const QJSValue &fieldsValue, const QString &acceptText, const QString &cancelText, const QJSValue &parentValue) const {
            QJSValue error;
            const auto windowInterface = resolveWindow(parentValue, &error);
            if (!windowInterface) {
                return error;
            }
            const auto quickWindow = qobject_cast<QQuickWindow *>(windowInterface->window());
            if (!quickWindow || !quickWindow->contentItem()) {
                return windowFailure(QStringLiteral("unsupportedOperation"), ShellInterface::tr("The application window cannot host this form dialog."));
            }
            const auto fields = formFields(fieldsValue);
            QQmlComponent component(Core::RuntimeInterface::qmlEngine(), QStringLiteral("DiffScope.BatchProcess"), QStringLiteral("ShellFormDialog"));
            if (component.isError()) {
                qFatal() << component.errorString();
            }
            std::unique_ptr<QObject> dialog(component.createWithInitialProperties({
                {QStringLiteral("parent"), QVariant::fromValue(quickWindow->contentItem())},
                {QStringLiteral("title"), title},
                {QStringLiteral("descriptionText"), description},
                {QStringLiteral("fieldDefinitions"), fields},
                {QStringLiteral("acceptText"), acceptText},
                {QStringLiteral("cancelText"), cancelText},
            }));
            if (!dialog) {
                qFatal() << component.errorString();
            }
            positionDialog(dialog.get(), quickWindow);
            if (!execDialog(dialog.get())) {
                return success(QJSValue(QJSValue::NullValue));
            }
            const auto values = dialog->property("resultValues").toList();
            if (values.size() != fields.size()) {
                qFatal("ShellFormDialog returned an invalid result");
            }
            auto result = m_engine->newArray(static_cast<quint32>(values.size()));
            for (qsizetype index = 0; index < values.size(); ++index) {
                if (fields.at(index).toMap().value(QStringLiteral("type")).toString() == QStringLiteral("color")) {
                    const auto color = values.at(index).value<QColor>();
                    auto colorValue = m_engine->newObject();
                    colorValue.setProperty(QStringLiteral("red"), color.redF());
                    colorValue.setProperty(QStringLiteral("green"), color.greenF());
                    colorValue.setProperty(QStringLiteral("blue"), color.blueF());
                    colorValue.setProperty(QStringLiteral("alpha"), color.alphaF());
                    colorValue.setProperty(QStringLiteral("css"), cssColor(color));
                    result.setProperty(static_cast<quint32>(index), colorValue);
                } else {
                    result.setProperty(static_cast<quint32>(index), m_engine->toScriptValue(values.at(index)));
                }
            }
            return success(result);
        }

        Q_INVOKABLE QJSValue openFile(const QString &title, const QString &initialPath, const QJSValue &filters, bool multiple, const QJSValue &parentValue) const {
            QJSValue error;
            const auto windowInterface = resolveWindow(parentValue, &error);
            if (!windowInterface) {
                return error;
            }
            const auto dialogTitle = title.isEmpty() ? ShellInterface::tr("Open File") : title;
            const auto filterText = fileDialogFilters(filters);
            const auto selectedPaths = multiple
                ? QFileDialog::getOpenFileNames(windowInterface->invisibleCentralWidget(), dialogTitle, initialPath, filterText)
                : QStringList{QFileDialog::getOpenFileName(windowInterface->invisibleCentralWidget(), dialogTitle, initialPath, filterText)};
            if (selectedPaths.isEmpty() || selectedPaths.constFirst().isEmpty()) {
                return success(QJSValue(QJSValue::NullValue));
            }
            auto result = m_engine->newArray(static_cast<quint32>(selectedPaths.size()));
            for (qsizetype index = 0; index < selectedPaths.size(); ++index) {
                FileSystemError fileError;
                const auto handle = m_fileSystemAccessInterface->createAuthorizedFileHandle(m_context, selectedPaths.at(index), FileAccessMode::Read, &fileError);
                const auto value = handle.isValid() ? m_fileSystemAccessInterface->toJavaScriptValue(m_context, handle, &fileError) : QJSValue(QJSValue::UndefinedValue);
                if (!handle.isValid() || value.isUndefined()) {
                    return hostFailure(fileError.message.isEmpty() ? ShellInterface::tr("The selected file could not be authorized.") : fileError.message);
                }
                result.setProperty(static_cast<quint32>(index), value);
            }
            return success(multiple ? result : result.property(static_cast<quint32>(0)));
        }

        Q_INVOKABLE QJSValue saveFile(const QString &title, const QString &initialPath, const QString &suggestedName, const QJSValue &filters, const QJSValue &parentValue) const {
            QJSValue error;
            const auto windowInterface = resolveWindow(parentValue, &error);
            if (!windowInterface) {
                return error;
            }
            auto selectedInitialPath = initialPath;
            if (!suggestedName.isEmpty()) {
                if (selectedInitialPath.isEmpty()) {
                    selectedInitialPath = suggestedName;
                } else if (QFileInfo(selectedInitialPath).isDir() || selectedInitialPath.endsWith(QLatin1Char('/')) || selectedInitialPath.endsWith(QLatin1Char('\\'))) {
                    selectedInitialPath = QDir(selectedInitialPath).filePath(suggestedName);
                }
            }
            const auto path = QFileDialog::getSaveFileName(
                windowInterface->invisibleCentralWidget(),
                title.isEmpty() ? ShellInterface::tr("Save File") : title,
                selectedInitialPath,
                fileDialogFilters(filters)
            );
            if (path.isEmpty()) {
                return success(QJSValue(QJSValue::NullValue));
            }
            FileSystemError fileError;
            const auto handle = m_fileSystemAccessInterface->createAuthorizedFileHandle(m_context, path, FileAccessMode::Write, &fileError);
            const auto value = handle.isValid() ? m_fileSystemAccessInterface->toJavaScriptValue(m_context, handle, &fileError) : QJSValue(QJSValue::UndefinedValue);
            if (!handle.isValid() || value.isUndefined()) {
                return hostFailure(fileError.message.isEmpty() ? ShellInterface::tr("The selected file could not be authorized.") : fileError.message);
            }
            return success(value);
        }

        Q_INVOKABLE QJSValue selectDirectory(const QString &title, const QString &initialPath, bool recursive, const QString &access, const QJSValue &parentValue) const {
            QJSValue error;
            const auto windowInterface = resolveWindow(parentValue, &error);
            if (!windowInterface) {
                return error;
            }
            const auto path = QFileDialog::getExistingDirectory(
                windowInterface->invisibleCentralWidget(),
                title.isEmpty() ? ShellInterface::tr("Select Directory") : title,
                initialPath
            );
            if (path.isEmpty()) {
                return success(QJSValue(QJSValue::NullValue));
            }
            FileSystemError fileError;
            const auto handle = m_fileSystemAccessInterface->createAuthorizedDirectoryHandle(m_context, path, fileAccessMode(access), recursive, &fileError);
            const auto value = handle.isValid() ? m_fileSystemAccessInterface->toJavaScriptValue(m_context, handle, &fileError) : QJSValue(QJSValue::UndefinedValue);
            if (!handle.isValid() || value.isUndefined()) {
                return hostFailure(fileError.message.isEmpty() ? ShellInterface::tr("The selected directory could not be authorized.") : fileError.message);
            }
            return success(value);
        }

        Q_INVOKABLE QJSValue postNotification(const QString &title, const QString &message, const QString &severity, const QString &bubble, const QJSValue &windowValue, bool hasWindow) const {
            if (!actionIsExecuting()) {
                return windowFailure(QStringLiteral("unsupportedOperation"), ShellInterface::tr("Shell operations are only available while a script action is executing."));
            }
            const auto icon = messageIcon(severity);
            if (!hasWindow) {
                Core::CoreInterface::sendNotification(icon, title, message, globalBubbleMode(bubble));
                return success();
            }
            const auto windowBridge = qobject_cast<WindowBridge *>(windowValue.toQObject());
            const auto windowInterface = windowBridge && windowBridge->context() == m_context ? windowBridge->windowInterface() : nullptr;
            if (!isUsableWindow(windowInterface)) {
                return windowFailure(QStringLiteral("invalidWindow"), ShellInterface::tr("The application window handle is no longer valid."));
            }
            const auto projectWindow = qobject_cast<Core::ProjectWindowInterface *>(windowInterface);
            if (!projectWindow) {
                return windowFailure(QStringLiteral("unsupportedOperation"), ShellInterface::tr("This application window does not support window notifications."));
            }
            projectWindow->sendNotification(icon, title, message, projectBubbleMode(bubble));
            return success();
        }

    private:
        static bool execDialog(QObject *dialog) {
            QEventLoop eventLoop;
            connect(dialog, SIGNAL(accepted()), &eventLoop, SLOT(quit()));
            connect(dialog, SIGNAL(rejected()), &eventLoop, SLOT(quit()));
            QMetaObject::invokeMethod(dialog, "open");
            eventLoop.exec();
            return dialog->property("result").toInt() == 1;
        }

        static void positionDialog(QObject *dialog, QQuickWindow *window) {
            dialog->setProperty("x", window->width() / 2.0 - dialog->property("width").toDouble() / 2.0);
            const auto topMargin = window->property("popupTopMarginHint");
            if (topMargin.isValid()) {
                dialog->setProperty("y", topMargin);
            } else {
                dialog->setProperty("y", window->height() / 2.0 - dialog->property("height").toDouble() / 2.0);
            }
        }

        QVariantList formFields(const QJSValue &fieldsValue) const {
            QVariantList result;
            const auto length = fieldsValue.property(QStringLiteral("length")).toUInt();
            result.reserve(static_cast<qsizetype>(length));
            for (quint32 index = 0; index < length; ++index) {
                const auto field = fieldsValue.property(index);
                const auto type = field.property(QStringLiteral("type")).toString();
                QVariantMap value{
                    {QStringLiteral("type"), type},
                    {QStringLiteral("name"), field.property(QStringLiteral("name")).toString()},
                    {QStringLiteral("label"), field.property(QStringLiteral("label")).toString()},
                    {QStringLiteral("description"), field.property(QStringLiteral("description")).toString()},
                    {QStringLiteral("required"), field.property(QStringLiteral("required")).toBool()},
                };
                if (type == QStringLiteral("text")) {
                    value.insert(QStringLiteral("value"), field.property(QStringLiteral("value")).toString());
                    value.insert(QStringLiteral("placeholder"), field.property(QStringLiteral("placeholder")).toString());
                    value.insert(QStringLiteral("multiline"), field.property(QStringLiteral("multiline")).toBool());
                    value.insert(QStringLiteral("minLength"), field.property(QStringLiteral("minLength")).toInt());
                    value.insert(QStringLiteral("maxLength"), field.property(QStringLiteral("maxLength")).toInt());
                    value.insert(QStringLiteral("pattern"), field.property(QStringLiteral("pattern")).toString());
                } else if (type == QStringLiteral("integer") || type == QStringLiteral("number")) {
                    value.insert(QStringLiteral("value"), field.property(QStringLiteral("value")).toNumber());
                    value.insert(QStringLiteral("minimum"), field.property(QStringLiteral("minimum")).toNumber());
                    value.insert(QStringLiteral("maximum"), field.property(QStringLiteral("maximum")).toNumber());
                    value.insert(QStringLiteral("step"), field.property(QStringLiteral("step")).toNumber());
                    value.insert(QStringLiteral("decimals"), type == QStringLiteral("integer") ? 0 : field.property(QStringLiteral("decimals")).toInt());
                } else if (type == QStringLiteral("boolean")) {
                    value.insert(QStringLiteral("value"), field.property(QStringLiteral("value")).toBool());
                } else if (type == QStringLiteral("choice")) {
                    value.insert(QStringLiteral("value"), field.property(QStringLiteral("value")).toString());
                    const auto itemsValue = field.property(QStringLiteral("items"));
                    QVariantList items;
                    const auto itemCount = itemsValue.property(QStringLiteral("length")).toUInt();
                    items.reserve(static_cast<qsizetype>(itemCount));
                    for (quint32 itemIndex = 0; itemIndex < itemCount; ++itemIndex) {
                        const auto item = itemsValue.property(itemIndex);
                        items.append(QVariantMap{
                            {QStringLiteral("value"), item.property(QStringLiteral("value")).toString()},
                            {QStringLiteral("label"), item.property(QStringLiteral("label")).toString()},
                            {QStringLiteral("description"), item.property(QStringLiteral("description")).toString()},
                            {QStringLiteral("enabled"), item.property(QStringLiteral("enabled")).toBool()},
                        });
                    }
                    value.insert(QStringLiteral("items"), items);
                } else if (type == QStringLiteral("color")) {
                    const auto colorValue = field.property(QStringLiteral("value"));
                    value.insert(QStringLiteral("value"), QColor::fromRgbF(
                        colorValue.property(QStringLiteral("red")).toNumber(),
                        colorValue.property(QStringLiteral("green")).toNumber(),
                        colorValue.property(QStringLiteral("blue")).toNumber(),
                        colorValue.property(QStringLiteral("alpha")).toNumber()
                    ));
                    value.insert(QStringLiteral("alpha"), field.property(QStringLiteral("alpha")).toBool());
                }
                result.append(value);
            }
            return result;
        }

        static Core::CoreInterface::NotificationBubbleMode globalBubbleMode(const QString &bubble) {
            if (bubble == QStringLiteral("hidden")) {
                return Core::CoreInterface::DoNotShowBubble;
            }
            if (bubble == QStringLiteral("autoHide")) {
                return Core::CoreInterface::AutoHide;
            }
            return Core::CoreInterface::NormalBubble;
        }

        static Core::ProjectWindowInterface::NotificationBubbleMode projectBubbleMode(const QString &bubble) {
            if (bubble == QStringLiteral("hidden")) {
                return Core::ProjectWindowInterface::DoNotShowBubble;
            }
            if (bubble == QStringLiteral("autoHide")) {
                return Core::ProjectWindowInterface::AutoHide;
            }
            return Core::ProjectWindowInterface::NormalBubble;
        }

        QPointer<ShellModuleExtension> m_extension;
        FileSystemAccessInterface *m_fileSystemAccessInterface;
        QPointer<ScriptExecutionContext> m_context;
        QJSEngine *m_engine;
    };

    WindowBridge::WindowBridge(ShellModuleBridge *moduleBridge, Core::ActionWindowInterfaceBase *windowInterface, QObject *parent)
        : QObject(parent), m_moduleBridge(moduleBridge), m_windowInterface(windowInterface) {
    }

    bool WindowBridge::isValid() const {
        return m_moduleBridge && isUsableWindow(m_windowInterface.data());
    }

    bool WindowBridge::isActive() const {
        return isValid() && m_windowInterface->window()->isActive();
    }

    Core::ActionWindowInterfaceBase *WindowBridge::windowInterface() const {
        return isValid() ? m_windowInterface.data() : nullptr;
    }

    ScriptExecutionContext *WindowBridge::context() const {
        return m_moduleBridge ? m_moduleBridge->context() : nullptr;
    }

    QJSValue WindowBridge::raise() const {
        if (!m_moduleBridge) {
            return QJSValue(QJSValue::UndefinedValue);
        }
        return m_moduleBridge->raiseWindow(m_windowInterface.data());
    }

    ShellModuleExtension::ShellModuleExtension(BatchProcessInterface *batchProcessInterface, ShellInterface *shellInterface, FileSystemAccessInterface *fileSystemAccessInterface, QObject *parent)
        : QObject(parent), m_batchProcessInterface(batchProcessInterface), m_shellInterface(shellInterface), m_fileSystemAccessInterface(fileSystemAccessInterface) {
        Q_ASSERT(m_batchProcessInterface);
        Q_ASSERT(m_shellInterface);
        Q_ASSERT(m_fileSystemAccessInterface);
        m_shellInterface->d_func()->moduleExtension = this;
        const auto extensionRegistered = m_batchProcessInterface->registerEngineExtension(this, [this](QJSEngine *engine, ScriptExecutionContext *context) {
            installIntoEngine(engine, context);
        });
        const auto moduleRegistered = m_batchProcessInterface->registerModule(QString::fromLatin1(ModuleName), this, [this](ScriptExecutionContext *context) {
            return createModule(context);
        });
        if (!extensionRegistered || !moduleRegistered) {
            qFatal("Failed to register the Batch Process shell module");
        }
        qCDebug(lcShellModule) << "Registered shell module" << ModuleName;
    }

    ShellModuleExtension::~ShellModuleExtension() = default;

    void ShellModuleExtension::installIntoEngine(QJSEngine *engine, ScriptExecutionContext *context) {
        if (!engine || !context || context->engine() != engine) {
            qFatal("The Batch Process shell engine extension received an invalid execution context");
        }
        QFile moduleFile(QString::fromLatin1(ModuleResourcePath));
        if (!moduleFile.open(QIODevice::ReadOnly)) {
            qFatal() << "Failed to open the embedded Batch Process shell module:" << moduleFile.errorString();
        }
        const auto factory = engine->evaluate(QString::fromUtf8(moduleFile.readAll()), QString::fromLatin1(ModuleSourceUrl));
        if (factory.isError()) {
            qFatal() << "Failed to evaluate the embedded Batch Process shell module:" << javaScriptErrorText(factory);
        }
        if (!factory.isCallable()) {
            qFatal("The embedded Batch Process shell module did not return a factory function");
        }
        auto bridge = new ShellModuleBridge(this, m_fileSystemAccessInterface, context, engine, engine);
        const auto bundle = factory.call({engine->newQObject(bridge)});
        if (engine->hasError()) {
            qFatal() << "Failed to create the Batch Process shell module:" << javaScriptErrorText(engine->catchError());
        }
        if (bundle.isError()) {
            qFatal() << "Failed to create the Batch Process shell module:" << javaScriptErrorText(bundle);
        }
        if (!bundle.isObject() || !bundle.property(QStringLiteral("exports")).isObject() || !bundle.property(QStringLiteral("wrapWindow")).isCallable() || !bundle.property(QStringLiteral("unwrapWindow")).isCallable() || !bundle.property(QStringLiteral("bridge")).isObject()) {
            qFatal("The embedded Batch Process shell module factory returned an invalid helper bundle");
        }
        m_engineBundles.insert(engine, bundle);
        connect(context, &QObject::destroyed, this, [this, engine] {
            m_engineBundles.remove(engine);
        });
    }

    QJSValue ShellModuleExtension::createModule(ScriptExecutionContext *context) const {
        const auto exports = helper(context, QStringLiteral("exports"));
        if (!exports.isObject()) {
            qFatal("The embedded Batch Process shell module exports are unavailable");
        }
        return exports;
    }

    QJSValue ShellModuleExtension::helper(ScriptExecutionContext *context, const QString &name) const {
        if (!context || !context->engine()) {
            return QJSValue(QJSValue::UndefinedValue);
        }
        const auto bundle = m_engineBundles.value(context->engine());
        if (!bundle.isObject() || !bundle.hasProperty(name)) {
            return QJSValue(QJSValue::UndefinedValue);
        }
        return bundle.property(name);
    }

    QJSValue ShellModuleExtension::wrapWindow(ScriptExecutionContext *context, Core::ActionWindowInterfaceBase *windowInterface) const {
        if (!context || !context->engine() || !isUsableWindow(windowInterface)) {
            return QJSValue(QJSValue::UndefinedValue);
        }
        const auto wrap = helper(context, QStringLiteral("wrapWindow"));
        const auto moduleBridge = qobject_cast<ShellModuleBridge *>(helper(context, QStringLiteral("bridge")).toQObject());
        if (!wrap.isCallable() || !moduleBridge) {
            return QJSValue(QJSValue::UndefinedValue);
        }
        auto bridge = new WindowBridge(moduleBridge, windowInterface, context->engine());
        const auto result = wrap.call({context->engine()->newQObject(bridge)});
        if (context->engine()->hasError() || result.isError()) {
            if (context->engine()->hasError()) {
                qCWarning(lcShellModule) << "Failed to wrap an application window:" << javaScriptErrorText(context->engine()->catchError());
            }
            bridge->deleteLater();
            return QJSValue(QJSValue::UndefinedValue);
        }
        return result;
    }

    bool ShellModuleExtension::unwrapWindow(ScriptExecutionContext *context, const QJSValue &value, Core::ActionWindowInterfaceBase **windowInterface) const {
        if (!context || !context->engine() || !windowInterface) {
            return false;
        }
        const auto unwrap = helper(context, QStringLiteral("unwrapWindow"));
        if (!unwrap.isCallable()) {
            return false;
        }
        const auto result = unwrap.call({value});
        if (context->engine()->hasError() || result.isError()) {
            if (context->engine()->hasError()) {
                context->engine()->catchError();
            }
            return false;
        }
        const auto bridge = qobject_cast<WindowBridge *>(result.toQObject());
        const auto resolvedWindow = bridge && bridge->context() == context ? bridge->windowInterface() : nullptr;
        if (!isUsableWindow(resolvedWindow)) {
            return false;
        }
        *windowInterface = resolvedWindow;
        return true;
    }

}

#include "ShellModuleExtension.moc"
#include "moc_ShellModuleExtension.cpp"
