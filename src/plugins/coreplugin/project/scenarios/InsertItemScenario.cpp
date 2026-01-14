#include "InsertItemScenario.h"
#include "InsertItemScenario_p.h"

#include <algorithm>
#include <utility>

#include <QCursor>
#include <QEventLoop>
#include <QLoggingCategory>
#include <QQmlComponent>
#include <QQuickItem>
#include <QQuickWindow>
#include <QVariant>
#include <QtGlobal>

#include <CoreApi/runtimeinterface.h>

#include <opendspx/track.h>

#include <SVSCraftCore/MusicTimeline.h>

#include <dspxmodel/Label.h>
#include <dspxmodel/LabelSequence.h>
#include <dspxmodel/Model.h>
#include <dspxmodel/SelectionModel.h>
#include <dspxmodel/Timeline.h>
#include <dspxmodel/Track.h>
#include <dspxmodel/TrackList.h>

#include <coreplugin/DspxDocument.h>
#include <coreplugin/ProjectTimeline.h>

#include <transactional/TransactionController.h>

namespace Core {

    Q_STATIC_LOGGING_CATEGORY(lcInsertItemScenario, "diffscope.core.insertitemscenario")

    QObject *InsertItemScenarioPrivate::createAndPositionDialog(QQmlComponent *component, const QVariantMap &initialProperties) const {
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

    bool InsertItemScenarioPrivate::execDialog(QObject *dialog) const {
        QEventLoop eventLoop;
        QObject::connect(dialog, SIGNAL(accepted()), &eventLoop, SLOT(quit()));
        QObject::connect(dialog, SIGNAL(rejected()), &eventLoop, SLOT(quit()));
        QMetaObject::invokeMethod(dialog, "open");
        eventLoop.exec();
        return dialog->property("result").toInt() == 1;
    }

    InsertItemScenario::InsertItemScenario(QObject *parent)
        : QObject(parent), d_ptr(new InsertItemScenarioPrivate) {
        Q_D(InsertItemScenario);
        d->q_ptr = this;
    }

    InsertItemScenario::~InsertItemScenario() = default;

    QQuickWindow *InsertItemScenario::window() const {
        Q_D(const InsertItemScenario);
        return d->window;
    }

    void InsertItemScenario::setWindow(QQuickWindow *window) {
        Q_D(InsertItemScenario);
        if (d->window != window) {
            d->window = window;
            Q_EMIT windowChanged();
        }
    }

    ProjectTimeline *InsertItemScenario::projectTimeline() const {
        Q_D(const InsertItemScenario);
        return d->projectTimeline;
    }

    void InsertItemScenario::setProjectTimeline(ProjectTimeline *projectTimeline) {
        Q_D(InsertItemScenario);
        if (d->projectTimeline != projectTimeline) {
            d->projectTimeline = projectTimeline;
            Q_EMIT projectTimelineChanged();
        }
    }

    DspxDocument *InsertItemScenario::document() const {
        Q_D(const InsertItemScenario);
        return d->document;
    }

    void InsertItemScenario::setDocument(DspxDocument *document) {
        Q_D(InsertItemScenario);
        if (d->document != document) {
            d->document = document;
            Q_EMIT documentChanged();
        }
    }

    bool InsertItemScenario::shouldDialogPopupAtCursor() const {
        Q_D(const InsertItemScenario);
        return d->shouldDialogPopupAtCursor;
    }

    void InsertItemScenario::setShouldDialogPopupAtCursor(bool shouldDialogPopupAtCursor) {
        Q_D(InsertItemScenario);
        if (d->shouldDialogPopupAtCursor != shouldDialogPopupAtCursor) {
            d->shouldDialogPopupAtCursor = shouldDialogPopupAtCursor;
            Q_EMIT shouldDialogPopupAtCursorChanged();
        }
    }

    void InsertItemScenario::addTrack() const {
        Q_D(const InsertItemScenario);
        if (!d->document)
            return;
        auto model = d->document->model();
        auto trackList = model->tracks();
        dspx::Track *newTrack = nullptr;
        bool success = false;
        auto selectionModel = d->document->selectionModel();
        d->document->transactionController()->beginScopedTransaction(tr("Adding track"), [=, &newTrack, &success] {
            newTrack = model->createTrack();
            newTrack->fromQDspx(QDspx::Track{});
            newTrack->setName(tr("Unnamed track"));
            auto insertionIndex = trackList->size();
            do {
                auto currentTrack = qobject_cast<dspx::Track *>(selectionModel->currentItem());
                if (!currentTrack) {
                   break;
                }
                auto index = trackList->items().indexOf(currentTrack);
                if (index == -1) {
                   break;
                }
                insertionIndex = index;
            } while (false);
            if (!trackList->insertItem(insertionIndex, newTrack)) {
                model->destroyItem(newTrack);
                newTrack = nullptr;
                return false;
            }
            success = true;
            return true;
        }, [] {
            qCCritical(lcInsertItemScenario()) << "Failed to add track in exclusive transaction";
        });
        if (success && newTrack) {
            selectionModel->select(newTrack, dspx::SelectionModel::Select | dspx::SelectionModel::SetCurrentItem | dspx::SelectionModel::ClearPreviousSelection);
        }
    }

    void InsertItemScenario::insertTrack() const {
        Q_D(const InsertItemScenario);
        if (!d->document || !d->window)
            return;
        auto model = d->document->model();
        auto trackList = model->tracks();
        QQmlComponent component(RuntimeInterface::qmlEngine(), "DiffScope.Core", "InsertTrackDialog");
        QVariantMap properties;
        properties.insert("trackCount", trackList->size());
        properties.insert("insertionIndex", trackList->size());
        properties.insert("insertionCount", 1);
        auto dialog = d->createAndPositionDialog(&component, properties);
        if (!d->execDialog(dialog))
            return;
        auto insertionIndex = dialog->property("insertionIndex").toInt();
        auto insertionCount = dialog->property("insertionCount").toInt();
        insertionIndex = std::clamp(insertionIndex, 0, trackList->size());
        QList<dspx::Track *> newTracks;
        newTracks.reserve(insertionCount);
        bool success = false;
        d->document->transactionController()->beginScopedTransaction(tr("Inserting track"), [=, &newTracks, &success] {
            for (int i = 0; i < insertionCount; ++i) {
                auto track = model->createTrack();
                track->fromQDspx(QDspx::Track{});
                track->setName(tr("Unnamed track"));
                if (!trackList->insertItem(insertionIndex + i, track)) {
                    model->destroyItem(track);
                    return false;
                }
                newTracks.append(track);
            }
            success = true;
            return true;
        }, [] {
            qCCritical(lcInsertItemScenario()) << "Failed to insert track in exclusive transaction";
        });
        if (success && !newTracks.isEmpty()) {
            auto selectionModel = d->document->selectionModel();
            bool first = true;
            for (auto track : std::as_const(newTracks)) {
                auto command = dspx::SelectionModel::Select | dspx::SelectionModel::SetCurrentItem;
                if (first) {
                    command |= dspx::SelectionModel::ClearPreviousSelection;
                    first = false;
                }
                selectionModel->select(track, command);
            }
        }
    }

    void InsertItemScenario::insertLabel() const {
        Q_D(const InsertItemScenario);
        if (!d->document || !d->projectTimeline || !d->window)
            return;
        auto model = d->document->model();
        auto labelSequence = model->timeline()->labels();
        QQmlComponent component(RuntimeInterface::qmlEngine(), "DiffScope.Core", "InsertLabelDialog");
        QVariantMap properties;
        properties.insert("timeline", QVariant::fromValue(d->projectTimeline->musicTimeline()));
        properties.insert("labelPos", d->projectTimeline->position());
        properties.insert("labelText", QString());
        auto dialog = d->createAndPositionDialog(&component, properties);
        if (!d->execDialog(dialog))
            return;
        auto labelPos = dialog->property("labelPos").toInt();
        auto labelText = dialog->property("labelText").toString();
        dspx::Label *newLabel = nullptr;
        bool success = false;
        d->document->transactionController()->beginScopedTransaction(tr("Inserting label"), [=, &newLabel, &success] {
            newLabel = model->createLabel();
            newLabel->setPos(qMax(0, labelPos));
            newLabel->setText(labelText);
            if (!labelSequence->insertItem(newLabel)) {
                model->destroyItem(newLabel);
                newLabel = nullptr;
                return false;
            }
            success = true;
            return true;
        }, [] {
            qCCritical(lcInsertItemScenario()) << "Failed to insert label in exclusive transaction";
        });
        if (success && newLabel) {
            auto selectionModel = d->document->selectionModel();
            selectionModel->select(newLabel, dspx::SelectionModel::Select | dspx::SelectionModel::SetCurrentItem | dspx::SelectionModel::ClearPreviousSelection);
        }
    }

}

#include "moc_InsertItemScenario.cpp"