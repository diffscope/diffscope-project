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
#include <coreplugin/private/DocumentEditScenario_p.h>

#include <transactional/TransactionController.h>

namespace Core {

    Q_STATIC_LOGGING_CATEGORY(lcEditMetadataScenario, "diffscope.core.editmetadatascenario")

    EditMetadataScenario::EditMetadataScenario(QObject *parent)
        : DocumentEditScenario(parent), d_ptr(new EditMetadataScenarioPrivate) {
        Q_D(EditMetadataScenario);
        d->q_ptr = this;
    }

    EditMetadataScenario::~EditMetadataScenario() = default;

    void EditMetadataScenario::editMetadata() const {
        Q_D(const EditMetadataScenario);
        if (!document() || !window()) {
            return;
        }
        auto model = document()->model();
        auto global = model->global();
        QQmlComponent component(RuntimeInterface::qmlEngine(), "DiffScope.Core", "EditMetadataDialog");
        QVariantMap properties;
        properties.insert("name", global->name());
        properties.insert("author", global->author());
        properties.insert("centShift", std::clamp(global->centShift(), -50, 50));
        auto dialog = createAndPositionDialog(&component, properties);
        if (!DocumentEditScenarioPrivate::execDialog(dialog)) {
            return;
        }
        auto name = dialog->property("name").toString();
        auto author = dialog->property("author").toString();
        auto centShift = dialog->property("centShift").toInt();
        centShift = std::clamp(centShift, -50, 50);
        document()->transactionController()->beginScopedTransaction(tr("Editing metadata"), [=] {
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
