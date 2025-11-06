#include "OpenSaveProjectFileScenario.h"
#include "OpenSaveProjectFileScenario_p.h"

#include <QApplication>
#include <QFileDialog>
#include <QQmlComponent>
#include <QQuickWindow>
#include <QSettings>
#include <QStandardPaths>
#include <QTimer>
#include <QWindow>

#include <QtQuickTemplates2/private/qquickbutton_p.h>

#include <CoreApi/runtimeinterface.h>

#include <SVSCraftQuick/MessageBox.h>

#include <coreplugin/CoreInterface.h>
#include <coreplugin/DspxInspectorDialog.h>

namespace Core {

    OpenSaveProjectFileScenario::OpenSaveProjectFileScenario(QObject *parent) : QObject(parent), d_ptr(new OpenSaveProjectFileScenarioPrivate) {
        Q_D(OpenSaveProjectFileScenario);
        d->q_ptr = this;
    }

    OpenSaveProjectFileScenario::~OpenSaveProjectFileScenario() = default;

    QWindow *OpenSaveProjectFileScenario::window() const {
        Q_D(const OpenSaveProjectFileScenario);
        return d->window;
    }

    void OpenSaveProjectFileScenario::setWindow(QWindow *window) {
        Q_D(OpenSaveProjectFileScenario);
        if (d->window != window) {
            d->window = window;
            Q_EMIT windowChanged();
        }
    }

    QString OpenSaveProjectFileScenario::dspxFileFilter(bool withAllFiles) {
        auto dspxFileFilter = tr("DiffScope Project Exchange Format (*.dspx)");
        auto allFileFilter = tr("All Files (*)");
        return withAllFiles ? dspxFileFilter + ";;" + allFileFilter : dspxFileFilter;
    }

    QString OpenSaveProjectFileScenario::openProjectFile(const QString &defaultDir) const {
        Q_D(const OpenSaveProjectFileScenario);
        Q_UNUSED(d->window);
        auto settings = RuntimeInterface::settings();
        settings->beginGroup(staticMetaObject.className());
        auto defaultOpenDir = !defaultDir.isEmpty() ? defaultDir : settings->value(QStringLiteral("defaultOpenDir"), QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)).toString();
        settings->endGroup();
        auto path = QFileDialog::getOpenFileName(
            nullptr,
            {},
            defaultOpenDir,
            dspxFileFilter(true)
        );
        if (path.isEmpty())
            return {};
        settings->beginGroup(staticMetaObject.className());
        settings->setValue(QStringLiteral("defaultOpenDir"), QFileInfo(path).absolutePath());
        settings->endGroup();

        return path;
    }

    QString OpenSaveProjectFileScenario::saveProjectFile(const QString &defaultDir) const {
        Q_D(const OpenSaveProjectFileScenario);
        Q_UNUSED(d->window);
        auto settings = RuntimeInterface::settings();
        settings->beginGroup(staticMetaObject.className());
        auto defaultSaveDir = !defaultDir.isEmpty() ? defaultDir : settings->value(QStringLiteral("defaultSaveDir"), QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)).toString();
        settings->endGroup();

        auto path = QFileDialog::getSaveFileName(
            nullptr,
            {},
            defaultSaveDir,
            dspxFileFilter(true)
        );
        if (path.isEmpty())
            return {};
        settings->beginGroup(staticMetaObject.className());
        settings->setValue(QStringLiteral("defaultSaveDir"), QFileInfo(path).absolutePath());
        settings->endGroup();

        return path;
    }

    void OpenSaveProjectFileScenario::showOpenFailMessageBox(const QString &path, const QString &error) const {
        Q_D(const OpenSaveProjectFileScenario);
        SVS::MessageBox::critical(RuntimeInterface::qmlEngine(), d->window, tr("Failed to open file"), QStringLiteral("%1\n\n%2").arg(QDir::toNativeSeparators(path), error));
    }

    void OpenSaveProjectFileScenario::showSaveFailMessageBox(const QString &path, const QString &error) const {
        Q_D(const OpenSaveProjectFileScenario);
        SVS::MessageBox::critical(RuntimeInterface::qmlEngine(), d->window, tr("Failed to save file"), QStringLiteral("%1\n\n%2").arg(QDir::toNativeSeparators(path), error));
    }

    void OpenSaveProjectFileScenario::showDeserializationFailMessageBox(const QString &path) const {
        Q_D(const OpenSaveProjectFileScenario);
        QQmlComponent buttonComponent(RuntimeInterface::qmlEngine(), "SVSCraft.UIComponents", "Button");
        auto button = qobject_cast<QQuickButton *>(buttonComponent.create());
        Q_ASSERT(button);
        button->setText(tr("Open DSPX Inspector"));
        QQmlComponent component(RuntimeInterface::qmlEngine(), "SVSCraft.UIComponents", "MessageBoxDialog");
        std::unique_ptr<QQuickWindow> mb(qobject_cast<QQuickWindow *>(component.createWithInitialProperties(
            {{"text", tr("Failed to parse file content")},
             {"informativeText", tr("%1\n\nYou can check for problems in the file with DSPX Inspector.").arg(QDir::toNativeSeparators(path))},
             {"icon", SVS::SVSCraft::Critical},
             {"transientParent", QVariant::fromValue(d->window)},
             {"content", QVariant::fromValue(button)}}
        )));
        Q_ASSERT(mb);
        connect(button, &QQuickButton::clicked, [this, path, &mb] {
            mb->close();
            QTimer::singleShot(0, [this, path] {
                DspxInspectorDialog dialog;
                dialog.setPath(path);
                dialog.runCheck();
                dialog.exec();
            });
        });
        SVS::MessageBox::customExec(mb.get());
    }

    bool OpenSaveProjectFileScenario::confirmFileCreatedByAnotherApplication(const QString &name) const {
        Q_D(const OpenSaveProjectFileScenario);
        return SVS::MessageBox::question(
            RuntimeInterface::qmlEngine(),
            d->window,
            tr("File created with another application"),
            QStringLiteral("This project file was created with another application (%1). Some features may not be fully compatible or may behave differently.\n\nDo you want to continue opening it?")
                .arg(name.isEmpty() ? tr("name unknown") : name)
        ) == SVS::SVSCraft::Yes;
    }

    bool OpenSaveProjectFileScenario::confirmFileCreatedByIncompatibleVersion(const QString &version) const {
        Q_D(const OpenSaveProjectFileScenario);
        return SVS::MessageBox::question(
            RuntimeInterface::qmlEngine(),
            d->window,
            tr("File created with incompatible %1 version").arg(QApplication::applicationDisplayName()),
            QStringLiteral("This project file was created with an newer version or test version of %1 (%2). Some features may not be fully compatible or may behave differently.\n\nDo you want to continue opening it?")
                .arg(QApplication::applicationDisplayName(), version)
        ) == SVS::SVSCraft::Yes;
    }

    bool OpenSaveProjectFileScenario::confirmCustomCheckWarning(const QString &message) const {
        Q_D(const OpenSaveProjectFileScenario);
        return SVS::MessageBox::question(
            RuntimeInterface::qmlEngine(),
            d->window,
            tr("Additional check failed"),
            QStringLiteral("%1\n\nThe file can still be opened, but it may cause potential problems.\n\nDo you want to continue opening it?")
        ) == SVS::SVSCraft::Yes;
    }

}
