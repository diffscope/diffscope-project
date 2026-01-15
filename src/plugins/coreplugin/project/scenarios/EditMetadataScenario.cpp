#include "EditMetadataScenario.h"
#include "EditMetadataScenario_p.h"

#include <algorithm>

#include <QCursor>
#include <QEventLoop>
#include <QLoggingCategory>
#include <QQmlComponent>
#include <QQuickItem>
#include <QQuickWindow>
#include <QVariant>

#include <CoreApi/runtimeinterface.h>

#include <dspxmodel/Global.h>
#include <dspxmodel/Model.h>

#include <coreplugin/DspxDocument.h>

#include <transactional/TransactionController.h>

namespace Core {

    Q_STATIC_LOGGING_CATEGORY(lcEditMetadataScenario, "diffscope.core.editmetadatascenario")

    QObject *EditMetadataScenarioPrivate::createAndPositionDialog(QQmlComponent *component, const QVariantMap &initialProperties) const {
        if (component->isError()) {
            qFatal() << component->errorString();
        }
        QVariantMap properties = initialProperties;
        properties.insert("parent", QVariant::fromValue(window->contentItem()));
        auto dialog = component->createWithInitialProperties(properties);
        if (!dialog) {
            qFatal() << component->errorString();
        }
        auto width = dialog->property("width").toDouble();
        auto height = dialog->property("height").toDouble();
        if (shouldDialogPopupAtCursor) {
            auto pos = window->mapFromGlobal(QCursor::pos()).toPointF();
            dialog->setProperty("x", qBound(0.0, pos.x(), window->width() - width));
            dialog->setProperty("y", qBound(0.0, pos.y(), window->height() - height));
        } else {
            dialog->setProperty("x", window->width() / 2.0 - width / 2);
            if (auto popupTopMarginHint = window->property("popupTopMarginHint"); popupTopMarginHint.isValid()) {
                dialog->setProperty("y", popupTopMarginHint);
            } else {
                dialog->setProperty("y", window->height() / 2.0 - height / 2);
            }
        }
        return dialog;
    }

    bool EditMetadataScenarioPrivate::execDialog(QObject *dialog) const {
        QEventLoop eventLoop;
        QObject::connect(dialog, SIGNAL(accepted()), &eventLoop, SLOT(quit()));
        QObject::connect(dialog, SIGNAL(rejected()), &eventLoop, SLOT(quit()));
        QMetaObject::invokeMethod(dialog, "open");
        eventLoop.exec();
        return dialog->property("result").toInt() == 1;
    }

    EditMetadataScenario::EditMetadataScenario(QObject *parent)
        : QObject(parent), d_ptr(new EditMetadataScenarioPrivate) {
        Q_D(EditMetadataScenario);
        d->q_ptr = this;
    }

    EditMetadataScenario::~EditMetadataScenario() = default;

    QQuickWindow *EditMetadataScenario::window() const {
        Q_D(const EditMetadataScenario);
        return d->window;
    }

    void EditMetadataScenario::setWindow(QQuickWindow *window) {
        Q_D(EditMetadataScenario);
        if (d->window != window) {
            d->window = window;
            Q_EMIT windowChanged();
        }
    }

    DspxDocument *EditMetadataScenario::document() const {
        Q_D(const EditMetadataScenario);
        return d->document;
    }

    void EditMetadataScenario::setDocument(DspxDocument *document) {
        Q_D(EditMetadataScenario);
        if (d->document != document) {
            d->document = document;
            Q_EMIT documentChanged();
        }
    }

    bool EditMetadataScenario::shouldDialogPopupAtCursor() const {
        Q_D(const EditMetadataScenario);
        return d->shouldDialogPopupAtCursor;
    }

    void EditMetadataScenario::setShouldDialogPopupAtCursor(bool shouldDialogPopupAtCursor) {
        Q_D(EditMetadataScenario);
        if (d->shouldDialogPopupAtCursor != shouldDialogPopupAtCursor) {
            d->shouldDialogPopupAtCursor = shouldDialogPopupAtCursor;
            Q_EMIT shouldDialogPopupAtCursorChanged();
        }
    }

    void EditMetadataScenario::editMetadata() const {
        Q_D(const EditMetadataScenario);
        if (!d->document || !d->window) {
            return;
        }
        auto model = d->document->model();
        auto global = model->global();
        QQmlComponent component(RuntimeInterface::qmlEngine(), "DiffScope.Core", "EditMetadataDialog");
        QVariantMap properties;
        properties.insert("name", global->name());
        properties.insert("author", global->author());
        properties.insert("centShift", std::clamp(global->centShift(), -50, 50));
        auto dialog = d->createAndPositionDialog(&component, properties);
        if (!d->execDialog(dialog)) {
            return;
        }
        auto name = dialog->property("name").toString();
        auto author = dialog->property("author").toString();
        auto centShift = dialog->property("centShift").toInt();
        centShift = std::clamp(centShift, -50, 50);
        d->document->transactionController()->beginScopedTransaction(tr("Editing metadata"), [=] {
            global->setName(name);
            global->setAuthor(author);
            global->setCentShift(centShift);
            return true;
        }, [] {
            qCCritical(lcEditMetadataScenario()) << "Failed to edit metadata in exclusive transaction";
        });
    }

}

#include "moc_EditMetadataScenario.cpp"
