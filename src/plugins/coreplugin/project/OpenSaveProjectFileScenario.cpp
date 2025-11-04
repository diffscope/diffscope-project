#include "OpenSaveProjectFileScenario.h"
#include "OpenSaveProjectFileScenario_p.h"

#include <QFileDialog>
#include <QQmlComponent>
#include <QQuickWindow>
#include <QStandardPaths>
#include <QWindow>

#include <QtQuickTemplates2/private/qquickbutton_p.h>

#include <CoreApi/runtimeinterface.h>

#include <SVSCraftQuick/MessageBox.h>

#include <coreplugin/CoreInterface.h>

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
            {{"text", tr("Failed to Parse File Content")},
             {"informativeText", tr("%1\n\nYou can check for problems in the file with DSPX Inspector.").arg(QDir::toNativeSeparators(path))},
             {"icon", SVS::SVSCraft::Critical},
             {"transientParent", QVariant::fromValue(d->window)},
             {"content", QVariant::fromValue(button)}}
        )));
        Q_ASSERT(mb);
        connect(button, &QQuickButton::clicked, [this, path, &mb] {
            mb->close();
            // TODO open DSPX Inspector
        });
        SVS::MessageBox::customExec(mb.get());
    }

}
