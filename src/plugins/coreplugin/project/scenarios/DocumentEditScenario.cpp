#include "DocumentEditScenario.h"
#include "DocumentEditScenario_p.h"

#include <QCursor>
#include <QQmlComponent>
#include <QQuickWindow>
#include <QQuickItem>

#include <coreplugin/DspxDocument.h>

namespace Core {

    bool DocumentEditScenarioPrivate::execDialog(QObject *dialog) {
        QEventLoop eventLoop;
        QObject::connect(dialog, SIGNAL(accepted()), &eventLoop, SLOT(quit()));
        QObject::connect(dialog, SIGNAL(rejected()), &eventLoop, SLOT(quit()));
        QMetaObject::invokeMethod(dialog, "open");
        eventLoop.exec();
        return dialog->property("result").toInt() == 1;
    }

    DocumentEditScenario::DocumentEditScenario(QObject *parent)
        : DocumentEditScenario(*new DocumentEditScenarioPrivate(), parent) {
    }

    DocumentEditScenario::DocumentEditScenario(DocumentEditScenarioPrivate &d, QObject *parent)
        : QObject(parent), d_ptr(&d) {
        d.q_ptr = this;
    }

    DocumentEditScenario::~DocumentEditScenario() = default;

    QQuickWindow *DocumentEditScenario::window() const {
        Q_D(const DocumentEditScenario);
        return d->window;
    }

    void DocumentEditScenario::setWindow(QQuickWindow *window) {
        Q_D(DocumentEditScenario);
        if (d->window != window) {
            d->window = window;
            Q_EMIT windowChanged();
        }
    }

    DspxDocument *DocumentEditScenario::document() const {
        Q_D(const DocumentEditScenario);
        return d->document;
    }

    void DocumentEditScenario::setDocument(DspxDocument *document) {
        Q_D(DocumentEditScenario);
        if (d->document != document) {
            d->document = document;
            Q_EMIT documentChanged();
        }
    }

    bool DocumentEditScenario::shouldDialogPopupAtCursor() const {
        Q_D(const DocumentEditScenario);
        return d->shouldDialogPopupAtCursor;
    }

    void DocumentEditScenario::setShouldDialogPopupAtCursor(bool shouldDialogPopupAtCursor) {
        Q_D(DocumentEditScenario);
        if (d->shouldDialogPopupAtCursor != shouldDialogPopupAtCursor) {
            d->shouldDialogPopupAtCursor = shouldDialogPopupAtCursor;
            Q_EMIT shouldDialogPopupAtCursorChanged();
        }
    }

    QObject *DocumentEditScenario::createAndPositionDialog(QQmlComponent *component, const QVariantMap &initialProperties) const {
        if (component->isError()) {
            qFatal() << component->errorString();
        }
        QVariantMap properties = initialProperties;
        properties.insert("parent", QVariant::fromValue(window()->contentItem()));
        auto dialog = component->createWithInitialProperties(properties);
        if (!dialog) {
            qFatal() << component->errorString();
        }
        auto width = dialog->property("width").toDouble();
        auto height = dialog->property("height").toDouble();
        if (shouldDialogPopupAtCursor()) {
            auto pos = window()->mapFromGlobal(QCursor::pos()).toPointF();
            dialog->setProperty("x", qBound(0.0, pos.x(), window()->width() - width));
            dialog->setProperty("y", qBound(0.0, pos.y(), window()->height() - height));
        } else {
            dialog->setProperty("x", window()->width() / 2.0 - width / 2);
            if (auto popupTopMarginHint = window()->property("popupTopMarginHint"); popupTopMarginHint.isValid()) {
                dialog->setProperty("y", popupTopMarginHint);
            } else {
                dialog->setProperty("y", window()->height() / 2.0 - height / 2);
            }
        }
        return dialog;
    }

}

#include "moc_DocumentEditScenario.cpp"
