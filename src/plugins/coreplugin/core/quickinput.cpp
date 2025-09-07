#include "quickinput.h"
#include "quickinput_p.h"

#include <QEventLoop>
#include <QTimer>
#include <QMetaObject>
#include <QWindow>

#include <coreplugin/iprojectwindow.h>

namespace Core {

    QuickInput::QuickInput(QObject *parent)
        : QObject(parent), d_ptr(new QuickInputPrivate) {
        Q_D(QuickInput);
        d->q_ptr = this;
    }

    QuickInput::~QuickInput() {
        Q_D(QuickInput);
        if (d->visible) {
            reject();
        }
    }

    QString QuickInput::placeholderText() const {
        Q_D(const QuickInput);
        return d->placeholderText;
    }

    void QuickInput::setPlaceholderText(const QString &placeholderText) {
        Q_D(QuickInput);
        if (d->placeholderText != placeholderText) {
            d->placeholderText = placeholderText;
            // Sync to InputPalette if connected
            if (d->inputPalette) {
                d->inputPalette->setProperty("placeholderText", placeholderText);
            }
            Q_EMIT placeholderTextChanged(placeholderText);
        }
    }

    QString QuickInput::promptText() const {
        Q_D(const QuickInput);
        return d->promptText;
    }

    void QuickInput::setPromptText(const QString &promptText) {
        Q_D(QuickInput);
        if (d->promptText != promptText) {
            d->promptText = promptText;
            // Sync to InputPalette if connected
            if (d->inputPalette) {
                d->inputPalette->setProperty("promptText", promptText);
            }
            Q_EMIT promptTextChanged(promptText);
        }
    }

    QString QuickInput::text() const {
        Q_D(const QuickInput);
        return d->text;
    }

    void QuickInput::setText(const QString &text) {
        Q_D(QuickInput);
        if (d->text != text) {
            d->text = text;
            // Sync to InputPalette if connected
            if (d->inputPalette) {
                d->inputPalette->setProperty("text", text);
            }
            Q_EMIT textChanged(text);
        }
    }

    SVS::SVSCraft::ControlType QuickInput::status() const {
        Q_D(const QuickInput);
        return d->status;
    }

    void QuickInput::setStatus(SVS::SVSCraft::ControlType status) {
        Q_D(QuickInput);
        if (d->status != status) {
            d->status = status;
            // Sync to InputPalette if connected
            if (d->inputPalette) {
                d->inputPalette->setProperty("status", QVariant::fromValue(status));
            }
            Q_EMIT statusChanged(status);
        }
    }

    bool QuickInput::acceptable() const {
        Q_D(const QuickInput);
        return d->acceptable;
    }

    void QuickInput::setAcceptable(bool acceptable) {
        Q_D(QuickInput);
        if (d->acceptable != acceptable) {
            d->acceptable = acceptable;
            // Sync to InputPalette if connected
            if (d->inputPalette) {
                d->inputPalette->setProperty("acceptable", acceptable);
            }
            Q_EMIT acceptableChanged(acceptable);
        }
    }

    IWindow *QuickInput::windowHandle() const {
        Q_D(const QuickInput);
        return d->windowHandle;
    }

    void QuickInput::setWindowHandle(IWindow *windowHandle) {
        Q_D(QuickInput);
        if (d->windowHandle != windowHandle) {
            // If currently visible, need to clean up InputPalette in old window first
            if (d->visible && d->inputPalette) {
                QMetaObject::invokeMethod(d->inputPalette, "close");
                d->clearInputPalette();
            }
            
            d->windowHandle = windowHandle;
            Q_EMIT windowHandleChanged(windowHandle);
            
            // If should be visible, open in new window
            if (d->visible && windowHandle) {
                show();
            }
        }
    }

    bool QuickInput::visible() const {
        Q_D(const QuickInput);
        return d->visible;
    }

    void QuickInput::setVisible(bool visible) {
        Q_D(QuickInput);
        if (d->visible != visible) {
            if (visible) {
                show();
            } else {
                reject();
            }
        }
    }

    void QuickInput::show() {
        Q_D(QuickInput);
        
        // Check windowHandle
        if (!d->windowHandle) {
            Q_EMIT rejected();
            Q_EMIT finished(QVariant());
            return;
        }
        
        // Get window object
        QWindow *window = d->windowHandle->window();
        if (!window) {
            Q_EMIT rejected();
            Q_EMIT finished(QVariant());
            return;
        }
        
        // Get InputPalette object
        QObject *inputPalette = window->property("inputPalette").value<QObject *>();
        if (!inputPalette) {
            Q_EMIT rejected();
            Q_EMIT finished(QVariant());
            return;
        }
        
        // If InputPalette is already open, close it first
        bool isVisible = inputPalette->property("visible").toBool();
        if (isVisible) {
            QMetaObject::invokeMethod(inputPalette, "close");
        }
        
        // Clean up old connections
        d->clearInputPalette();
        
        // Set new InputPalette
        d->inputPalette = inputPalette;
        d->connectInputPalette();
        d->syncToInputPalette();
        
        // Listen for windowHandle destroyed signal
        connect(d->windowHandle, &QObject::destroyed, this, &QuickInput::handleWindowHandleDestroyed, Qt::UniqueConnection);
        
        // Set visible state
        if (!d->visible) {
            d->visible = true;
            Q_EMIT visibleChanged(true);
        }
        
        // Delay opening InputPalette
        QTimer::singleShot(0, [inputPalette]() {
            QMetaObject::invokeMethod(inputPalette, "open");
        });
    }

    QVariant QuickInput::exec() {
        QEventLoop eventLoop;
        QVariant result;
        connect(this, &QuickInput::finished, [&eventLoop, &result](const QVariant &res) {
            result = res;
            eventLoop.exit();
        });
        show();
        eventLoop.exec();
        return result;
    }

    void QuickInput::accept() {
        Q_D(QuickInput);
        if (d->inputPalette) {
            QMetaObject::invokeMethod(d->inputPalette, "accept");
        } else {
            done(QVariant(d->text));
        }
    }

    void QuickInput::done(const QVariant &result) {
        Q_D(QuickInput);
        d->clearInputPalette();
        if (d->visible) {
            d->visible = false;
            Q_EMIT visibleChanged(false);
        }
        Q_EMIT finished(result);
    }

    void QuickInput::reject() {
        Q_D(QuickInput);
        if (d->inputPalette) {
            QMetaObject::invokeMethod(d->inputPalette, "close");
        } else {
            done(QVariant());
        }
    }

    void QuickInputPrivate::connectInputPalette() {
        Q_Q(QuickInput);
        if (!inputPalette) return;
        
        // Connect InputPalette signals
        q->connect(inputPalette, SIGNAL(accepted()), q, SIGNAL(accepted()));
        q->connect(inputPalette, SIGNAL(rejected()), q, SLOT(reject()));
        q->connect(inputPalette, SIGNAL(finished(QVariant)), q, SLOT(done(QVariant)));
        q->connect(inputPalette, SIGNAL(attemptingAcceptButFailed()), q, SIGNAL(attemptingAcceptButFailed()));
        
        // Connect property change signals for bidirectional sync
        q->connect(inputPalette, SIGNAL(textChanged()), q, SLOT(updateFromInputPalette()));
    }

    void QuickInputPrivate::disconnectInputPalette() {
        Q_Q(QuickInput);
        if (!inputPalette) return;
        
        // Disconnect all connections
        QObject::disconnect(inputPalette, nullptr, q, nullptr);
    }

    void QuickInputPrivate::syncToInputPalette() {
        if (!inputPalette) return;
        
        // Sync properties to InputPalette
        inputPalette->setProperty("placeholderText", placeholderText);
        inputPalette->setProperty("promptText", promptText);
        inputPalette->setProperty("text", text);
        inputPalette->setProperty("status", QVariant::fromValue(status));
        inputPalette->setProperty("acceptable", acceptable);
    }

    void QuickInputPrivate::clearInputPalette() {
        if (inputPalette) {
            disconnectInputPalette();
            inputPalette = nullptr;
        }
    }

    void QuickInput::updateFromInputPalette() {
        Q_D(QuickInput);
        if (!d->inputPalette) return;
        
        // Read properties from InputPalette and update, avoiding circular updates
        QString newText = d->inputPalette->property("text").toString();
        if (d->text != newText) {
            d->text = newText;
            Q_EMIT textChanged(newText);
        }
    }

    void QuickInput::handleWindowHandleDestroyed() {
        Q_D(QuickInput);
        d->clearInputPalette();
        if (d->visible) {
            d->visible = false;
            Q_EMIT visibleChanged(false);
            Q_EMIT rejected();
            Q_EMIT finished(QVariant());
        }
    }

}

#include "moc_quickinput.cpp"
