#include "quickpick.h"
#include "quickpick_p.h"

#include <QAbstractItemModel>
#include <QEventLoop>
#include <QMetaObject>
#include <QTimer>

namespace Core {

    QuickPick::QuickPick(QObject *parent)
        : QObject(parent), d_ptr(new QuickPickPrivate) {
        Q_D(QuickPick);
        d->q_ptr = this;
    }

    QuickPick::~QuickPick() {
        Q_D(QuickPick);
        if (d->visible) {
            reject();
        }
    }

    QAbstractItemModel *QuickPick::model() const {
        Q_D(const QuickPick);
        return d->model;
    }

    void QuickPick::setModel(QAbstractItemModel *model) {
        Q_D(QuickPick);
        if (d->model != model) {
            d->model = model;
            // Sync to CommandPalette if connected
            if (d->commandPalette) {
                d->commandPalette->setProperty("model", QVariant::fromValue(model));
            }
            Q_EMIT modelChanged(model);
        }
    }

    QString QuickPick::filterText() const {
        Q_D(const QuickPick);
        return d->filterText;
    }

    void QuickPick::setFilterText(const QString &filterText) {
        Q_D(QuickPick);
        if (d->filterText != filterText) {
            d->filterText = filterText;
            // Sync to CommandPalette if connected
            if (d->commandPalette) {
                d->commandPalette->setProperty("filterText", filterText);
            }
            Q_EMIT filterTextChanged(filterText);
        }
    }

    QString QuickPick::placeholderText() const {
        Q_D(const QuickPick);
        return d->placeholderText;
    }

    void QuickPick::setPlaceholderText(const QString &placeholderText) {
        Q_D(QuickPick);
        if (d->placeholderText != placeholderText) {
            d->placeholderText = placeholderText;
            // Sync to CommandPalette if connected
            if (d->commandPalette) {
                d->commandPalette->setProperty("placeholderText", placeholderText);
            }
            Q_EMIT placeholderTextChanged(placeholderText);
        }
    }

    int QuickPick::currentIndex() const {
        Q_D(const QuickPick);
        return d->currentIndex;
    }

    void QuickPick::setCurrentIndex(int currentIndex) {
        Q_D(QuickPick);
        if (d->currentIndex != currentIndex) {
            d->currentIndex = currentIndex;
            // Sync to CommandPalette if connected
            if (d->commandPalette) {
                d->commandPalette->setProperty("currentIndex", currentIndex);
            }
            Q_EMIT currentIndexChanged(currentIndex);
        }
    }

    WindowInterface *QuickPick::windowHandle() const {
        Q_D(const QuickPick);
        return d->windowHandle;
    }

    void QuickPick::setWindowHandle(WindowInterface *windowHandle) {
        Q_D(QuickPick);
        if (d->windowHandle != windowHandle) {
            // If currently visible, need to clean up CommandPalette in old window first
            if (d->visible && d->commandPalette) {
                QMetaObject::invokeMethod(d->commandPalette, "close");
                d->clearCommandPalette();
            }

            d->windowHandle = windowHandle;
            Q_EMIT windowHandleChanged(windowHandle);

            // If should be visible, open in new window
            if (d->visible && windowHandle) {
                show();
            }
        }
    }

    bool QuickPick::visible() const {
        Q_D(const QuickPick);
        return d->visible;
    }

    void QuickPick::setVisible(bool visible) {
        Q_D(QuickPick);
        if (d->visible != visible) {
            if (visible) {
                show();
            } else {
                reject();
            }
        }
    }

    void QuickPick::show() {
        Q_D(QuickPick);

        // Check windowHandle
        if (!d->windowHandle) {
            Q_EMIT rejected();
            Q_EMIT finished(-1);
            return;
        }

        // Get window object
        QWindow *window = d->windowHandle->window();
        if (!window) {
            Q_EMIT rejected();
            Q_EMIT finished(-1);
            return;
        }

        // Get CommandPalette object
        QObject *commandPalette = window->property("commandPalette").value<QObject *>();
        if (!commandPalette) {
            Q_EMIT rejected();
            Q_EMIT finished(-1);
            return;
        }

        // If CommandPalette is already open, close it first
        bool isVisible = commandPalette->property("visible").toBool();
        if (isVisible) {
            QMetaObject::invokeMethod(commandPalette, "close");
        }

        // Clean up old connections
        d->clearCommandPalette();

        // Set new CommandPalette
        d->commandPalette = commandPalette;
        d->connectCommandPalette();
        d->syncToCommandPalette();

        // Listen for windowHandle destroyed signal
        connect(d->windowHandle, &QObject::destroyed, this, &QuickPick::handleWindowHandleDestroyed, Qt::UniqueConnection);

        // Set visible state
        if (!d->visible) {
            d->visible = true;
            Q_EMIT visibleChanged(true);
        }

        // Delay opening CommandPalette
        QTimer::singleShot(0, [commandPalette]() {
            QMetaObject::invokeMethod(commandPalette, "open");
        });
    }

    int QuickPick::exec() {
        QEventLoop eventLoop;
        connect(this, &QuickPick::finished, &eventLoop, &QEventLoop::exit);
        show();
        return eventLoop.exec();
    }

    void QuickPick::accept() {
        Q_D(QuickPick);
        if (d->commandPalette) {
            QMetaObject::invokeMethod(d->commandPalette, "accept");
        } else {
            done(d->currentIndex);
        }
    }

    void QuickPick::done(int result) {
        Q_D(QuickPick);
        d->clearCommandPalette();
        if (d->visible) {
            d->visible = false;
            Q_EMIT visibleChanged(false);
        }
        Q_EMIT finished(result);
    }

    void QuickPick::reject() {
        Q_D(QuickPick);
        if (d->commandPalette) {
            QMetaObject::invokeMethod(d->commandPalette, "close");
        } else {
            done(-1);
        }
    }

    void QuickPickPrivate::connectCommandPalette() {
        Q_Q(QuickPick);
        if (!commandPalette)
            return;

        // Connect CommandPalette signals
        q->connect(commandPalette, SIGNAL(accepted()), q, SIGNAL(accepted()));
        q->connect(commandPalette, SIGNAL(rejected()), q, SLOT(reject()));
        q->connect(commandPalette, SIGNAL(finished(int)), q, SLOT(done(int)));

        // Connect property change signals for bidirectional sync
        q->connect(commandPalette, SIGNAL(filterTextChanged()), q, SLOT(updateFromCommandPalette()));
        q->connect(commandPalette, SIGNAL(currentIndexChanged()), q, SLOT(updateFromCommandPalette()));
    }

    void QuickPickPrivate::disconnectCommandPalette() {
        Q_Q(QuickPick);
        if (!commandPalette)
            return;

        // Disconnect all connections
        QObject::disconnect(commandPalette, nullptr, q, nullptr);
    }

    void QuickPickPrivate::syncToCommandPalette() {
        if (!commandPalette)
            return;
        // Sync properties to CommandPalette
        commandPalette->setProperty("model", QVariant::fromValue(model));
        commandPalette->setProperty("filterText", filterText);
        commandPalette->setProperty("placeholderText", placeholderText);
        commandPalette->setProperty("currentIndex", currentIndex);
    }

    void QuickPickPrivate::clearCommandPalette() {
        if (commandPalette) {
            disconnectCommandPalette();
            commandPalette->setProperty("model", QVariant::fromValue(nullptr));
            commandPalette->setProperty("filterText", "");
            commandPalette->setProperty("placeholderText", "");
            commandPalette->setProperty("currentIndex", 0);
            commandPalette = nullptr;
        }
    }

    void QuickPick::updateFromCommandPalette() {
        Q_D(QuickPick);
        if (!d->commandPalette)
            return;

        // Read properties from CommandPalette and update, avoiding circular updates
        QString newFilterText = d->commandPalette->property("filterText").toString();
        if (d->filterText != newFilterText) {
            d->filterText = newFilterText;
            Q_EMIT filterTextChanged(newFilterText);
        }

        int newCurrentIndex = d->commandPalette->property("currentIndex").toInt();
        if (d->currentIndex != newCurrentIndex) {
            d->currentIndex = newCurrentIndex;
            Q_EMIT currentIndexChanged(newCurrentIndex);
        }
    }
    void QuickPick::handleWindowHandleDestroyed() {
        Q_D(QuickPick);
        d->clearCommandPalette();
        if (d->visible) {
            d->visible = false;
            Q_EMIT visibleChanged(false);
            Q_EMIT rejected();
            Q_EMIT finished(-1);
        }
    }

}

#include "moc_quickpick.cpp"
