#include "DspxInspectorDialog.h"
#include "DspxInspectorDialog_p.h"

#include <application_config.h>

#include <QApplication>
#include <QDir>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QTreeView>
#include <QStandardItemModel>
#include <QTabWidget>

#include <opendspx/model.h>
#include <opendspxserializer/serializer.h>

#include <SVSCraftCore/Semver.h>

#include <coreplugin/CoreInterface.h>
#include <coreplugin/DspxCheckerRegistry.h>
#include <coreplugin/OpenSaveProjectFileScenario.h>

namespace Core {

    static bool checkIsVersionCompatible(const QString &version) {
        if (version.isEmpty())
            return true;
        SVS::Semver currentSemver(QStringLiteral(APPLICATION_SEMVER));
        SVS::Semver fileSemver(version);
        if (fileSemver == currentSemver)
            return true;
        if (fileSemver > currentSemver)
            return false;
        if (!fileSemver.preRelease().isEmpty() || !fileSemver.build().isEmpty()) {
            return false;
        }
        return true;
    }

    DspxInspectorDialog::DspxInspectorDialog(QWidget *parent) : QDialog(parent), d_ptr(new DspxInspectorDialogPrivate) {
        Q_D(DspxInspectorDialog);
        d->q_ptr = this;
        auto layout = new QVBoxLayout;
        setLayout(layout);
        auto browseLayout = new QHBoxLayout;
        layout->addLayout(browseLayout);
        auto pathEdit = new QLineEdit;
        browseLayout->addWidget(pathEdit);
        auto browseButton = new QPushButton(tr("Browse"));
        browseLayout->addWidget(browseButton);
        auto runCheckButton = new QPushButton(tr("Run Check"));
        runCheckButton->setEnabled(false);
        layout->addWidget(runCheckButton);
        auto tabWidget = new QTabWidget;
        layout->addWidget(tabWidget);
        auto fileStructureTreeView = new QTreeView;
        fileStructureTreeView->setHeaderHidden(true);
        tabWidget->addTab(fileStructureTreeView, tr("File Structure"));
        auto problemTreeView = new QTreeView;
        problemTreeView->setHeaderHidden(true);
        tabWidget->addTab(problemTreeView, tr("Problems"));

        d->tabWidget = tabWidget;
        d->problemTreeView = problemTreeView;
        d->fileStructureTreeView = fileStructureTreeView;
        resize(640, 480);
        setWindowTitle(tr("DSPX Inspector"));

        connect(pathEdit, &QLineEdit::textChanged, this, &DspxInspectorDialog::setPath);
        connect(this, &DspxInspectorDialog::pathChanged, pathEdit, &QLineEdit::setText);
        connect(this, &DspxInspectorDialog::pathChanged, runCheckButton, [runCheckButton](const QString &path) {
            runCheckButton->setEnabled(!path.isEmpty());
        });


        auto scenario = new OpenSaveProjectFileScenario(this);
        connect(browseButton, &QPushButton::clicked, [scenario, this] {
            auto path = scenario->openProjectFile();
            if (!path.isEmpty())
                setPath(path);
        });

        connect(runCheckButton, &QPushButton::clicked, this, &DspxInspectorDialog::runCheck);
    }

    DspxInspectorDialog::~DspxInspectorDialog() = default;

    QString DspxInspectorDialog::path() const {
        Q_D(const DspxInspectorDialog);
        return d->path;
    }

    void DspxInspectorDialog::setPath(const QString &path) {
        Q_D(DspxInspectorDialog);
        if (d->path != path) {
            d->path = QDir::toNativeSeparators(path);
            Q_EMIT pathChanged(d->path);
        }
    }

    static void addErrorItem(QStandardItemModel *model, const QString &message, const QString &jsonPath, const QIcon &icon, const QList<QPair<QString, QVariant>> &info, const QString &description = {}) {
        auto titleItem = new QStandardItem;
        titleItem->setText(message);
        titleItem->setIcon(icon);
        titleItem->setToolTip(description);
        titleItem->setEditable(false);
        auto pathItem = new QStandardItem;
        pathItem->setText(jsonPath);
        pathItem->setEditable(false);
        if (!jsonPath.isEmpty()) {
            pathItem->setToolTip("JSONPath");
        }
        model->appendRow({titleItem, pathItem});
        for (const auto &[key, value] : info) {
            auto keyItem = new QStandardItem;
            keyItem->setText(key);
            keyItem->setEditable(false);
            auto valueItem = new QStandardItem;
            valueItem->setData(value, Qt::DisplayRole);
            valueItem->setEditable(false);
            titleItem->appendRow({keyItem, valueItem});
        }
    }

    void DspxInspectorDialog::runCheck() {
        static const char *jsonDataTypeNames[] = {
            QT_TR_NOOP("Null"),
            QT_TR_NOOP("Boolean"),
            QT_TR_NOOP("Integer"),
            QT_TR_NOOP("Number"),
            QT_TR_NOOP("String"),
            QT_TR_NOOP("Array"),
            QT_TR_NOOP("Object"),
        };
        Q_D(DspxInspectorDialog);
        if (d->path.isEmpty())
            return;
        if (d->problemTreeView->model()) {
            d->problemTreeView->model()->deleteLater();
            d->problemTreeView->setModel(nullptr);
        }
        auto problemModel = new QStandardItemModel(this);
        d->problemTreeView->setModel(problemModel);
        QFile f(d->path);
        if (!f.open(QIODevice::ReadOnly)) {
            addErrorItem(
                problemModel,
                tr("Fatal: Failed to open file"),
                {},
                style()->standardIcon(QStyle::SP_MessageBoxCritical),
                {
                    {tr("Path"), d->path},
                    {tr("Error code"), f.error()},
                    {tr("Error text"), f.errorString()}
                }
            );
        } else {
            auto data = f.readAll();
            QDspx::SerializationErrorList errors;
            auto dspxModel = QDspx::Serializer::deserialize(data, errors, QDspx::Serializer::CheckError | QDspx::Serializer::CheckWarning);
            for (const auto &error : errors) {
                switch (error->type()) {
                    case QDspx::SerializationError::JsonParseFailure: {
                        auto e = error.staticCast<QDspx::JsonParseFailureError>();
                        addErrorItem(
                            problemModel,
                            tr("Fatal: Failed to parse JSON") + QStringLiteral(" (0x%1)").arg(QString::number(error->type(), 16), 4, '0'),
                            {},
                            style()->standardIcon(QStyle::SP_MessageBoxCritical),
                            {
                                {tr("Offset in file"), e->error().offset},
                                {tr("Error code"), e->error().error},
                                {tr("Error text"), e->error().errorString()},
                            },
                            tr("The file is not a valid JSON document.")
                        );
                        break;
                    }
                    case QDspx::SerializationError::JsonRootIsNotObject: {
                        addErrorItem(
                            problemModel,
                            tr("Fatal: Root is not an object") + QStringLiteral(" (0x%1)").arg(QString::number(error->type(), 16), 4, '0'),
                            QStringLiteral("$"),
                            style()->standardIcon(QStyle::SP_MessageBoxCritical),
                            {},
                            tr("The root of JSON document is not an object.")
                        );
                        break;
                    }
                    case QDspx::SerializationError::UnrecognizedVersion: {
                        auto e = error.staticCast<QDspx::UnrecognizedVersionError>();
                        addErrorItem(
                            problemModel,
                            tr("Fatal: Unrecognized version") + QStringLiteral(" (0x%1)").arg(QString::number(error->type(), 16), 4, '0'),
                            QStringLiteral("$.version"),
                            style()->standardIcon(QStyle::SP_MessageBoxCritical),
                            {
                                {tr("Actual version"), e->actualVersion()},
                            },
                            tr("This project file may have been created with a newer version of %1 or another application, and its version is not recognized by %1. Please try exporting the project from the application with which it was created as a version compatible with your current %1.").arg(QApplication::applicationDisplayName())
                        );
                        break;
                    }
                    case QDspx::SerializationError::InvalidDataType: {
                        auto e = error.staticCast<QDspx::InvalidDataTypeError>();
                        QStringList expectedTypeStrings;
                        for (auto expectedType : e->expectedTypes()) {
                            expectedTypeStrings.append(tr(jsonDataTypeNames[expectedType]));
                        }
                        addErrorItem(
                            problemModel,
                            tr("Invalid data type") + QStringLiteral(" (0x%1)").arg(QString::number(error->type(), 16), 4, '0'),
                            e->path(),
                            style()->standardIcon(QStyle::SP_MessageBoxCritical),
                            {
                                {tr("Expected data type"), expectedTypeStrings.join(tr(", "))},
                                {tr("Actual data type"), tr(jsonDataTypeNames[e->actualType()])},
                            },
                            tr("The value at the specific path is not of the expected data type.")
                        );
                        break;
                    }
                    case QDspx::SerializationError::InvalidObjectType: {
                        auto e = error.staticCast<QDspx::InvalidObjectTypeError>();
                        addErrorItem(
                            problemModel,
                            tr("Invalid object type") + QStringLiteral(" (0x%1)").arg(QString::number(error->type(), 16), 4, '0'),
                            e->path(),
                            style()->standardIcon(QStyle::SP_MessageBoxCritical),
                            {
                                {tr("Expected object type"), e->expectedTypes().join(QStringLiteral(", "))},
                                {tr("Actual object type"), e->actualType()},
                            },
                            tr("The object at the specific path is not of the expected object type.")
                        );
                        break;
                    }
                    case QDspx::SerializationError::RangeConstraintViolation: {
                        auto e = error.staticCast<QDspx::RangeConstraintViolationError>();
                        addErrorItem(
                            problemModel,
                            tr("Range constraint violation") + QStringLiteral(" (0x%1)").arg(QString::number(error->type(), 16), 4, '0'),
                            e->path(),
                            style()->standardIcon(QStyle::SP_MessageBoxCritical),
                            {
                                {tr("Expected maximum value"), e->expectedMaximum().isNull() ? tr("None") : e->expectedMaximum()},
                                {tr("Expected minimum value"), e->expectedMinimum().isNull() ? tr("None") : e->expectedMinimum()},
                                {tr("Actual value"), e->actualValue().toString()},
                            },
                            tr("The value of the property at the specific path is outside the allowed range. The value must be between the expected minimum and maximum value (inclusive).")
                        );
                        break;
                    }
                    case QDspx::SerializationError::EnumConstraintViolation: {
                        auto e = error.staticCast<QDspx::EnumConstraintViolationError>();
                        QList<QPair<QString, QVariant>> info;
                        for (const auto &expectedEnumValue : e->expectedEnumValues()) {
                            info.emplace_back(tr("Expected enum value"), expectedEnumValue.toString());
                        }
                        info.emplace_back(tr("Actual enum value"), e->actualEnumValue().toString());
                        addErrorItem(
                            problemModel,
                            tr("Enum constraint violation") + QStringLiteral(" (0x%1)").arg(QString::number(error->type(), 16), 4, '0'),
                            e->path(),
                            style()->standardIcon(QStyle::SP_MessageBoxCritical),
                            info,
                            tr("The value of the property at the specific path is not one of the allowed enum values.")
                        );
                        break;
                    }
                    case QDspx::SerializationError::MissingProperty: {
                        auto e = error.staticCast<QDspx::MissingPropertyError>();
                        QList<QPair<QString, QVariant>> info;
                        for (const auto &missingProperty : e->missingProperties()) {
                            info.emplace_back(tr("Missing property"), missingProperty);
                        }
                        addErrorItem(
                            problemModel,
                            tr("Missing properties") + QStringLiteral(" (0x%1)").arg(QString::number(error->type(), 16), 4, '0'),
                            e->path(),
                            style()->standardIcon(QStyle::SP_MessageBoxCritical),
                            info,
                            tr("One or more properties are missing in the object at the specific path.")
                        );
                        break;
                    }
                    case QDspx::SerializationError::RedundantProperty: {
                        auto e = error.staticCast<QDspx::MissingPropertyError>();
                        QList<QPair<QString, QVariant>> info;
                        for (const auto &redundantProperty : e->missingProperties()) {
                            info.emplace_back(tr("Redundant property"), redundantProperty);
                        }
                        addErrorItem(
                            problemModel,
                            tr("Redundant properties") + QStringLiteral(" (0x%1)").arg(QString::number(error->type(), 16), 4, '0'),
                            e->path(),
                            style()->standardIcon(QStyle::SP_MessageBoxCritical),
                            info,
                            tr("One or more properties are redundant in the object at the specific path.")
                        );
                        break;
                    }
                    case QDspx::SerializationError::OverlappingItem: {
                        auto e = error.staticCast<QDspx::OverlappingItemError>();
                        QList<QPair<QString, QVariant>> info;
                        for (auto overlappedItemIndex : e->overlappedItemIndexes()) {
                            info.emplace_back(tr("Index"), overlappedItemIndex);
                        }
                        addErrorItem(
                            problemModel,
                            tr("Overlapping items") + QStringLiteral(" (0x%1)").arg(QString::number(error->type(), 16), 4, '0'),
                            e->path(),
                            {},
                            info,
                            tr("Items at specific indexes in the array at the specific path overlap.")
                        );
                        break;
                    }
                    case QDspx::SerializationError::ZeroLengthRange: {
                        auto e = error.staticCast<QDspx::ZeroLengthRangeError>();
                        addErrorItem(
                            problemModel,
                            tr("Zero-length range") + QStringLiteral(" (0x%1)").arg(QString::number(error->type(), 16), 4, '0'),
                            e->path(),
                            {},
                            {},
                            tr("The range length of the entity object at the specific path is zero.")
                        );
                        break;
                    }
                    case QDspx::SerializationError::ErroneousClipRange: {
                        auto e = error.staticCast<QDspx::ErroneousClipRangeError>();
                        addErrorItem(
                            problemModel,
                            tr("Erroneous clip range") + QStringLiteral(" (0x%1)").arg(QString::number(error->type(), 16), 4, '0'),
                            e->path(),
                            {},
                            {},
                            tr("The clipping range of the clip entity object at the specific path exceeds its range limit.")
                        );
                        break;
                    }
                    case QDspx::SerializationError::ErroneousClipPosition: {
                        auto e = error.staticCast<QDspx::ErroneousClipPositionError>();
                        addErrorItem(
                            problemModel,
                            tr("Erroneous clip position") + QStringLiteral(" (0x%1)").arg(QString::number(error->type(), 16), 4, '0'),
                            e->path(),
                            {},
                            {},
                            tr("The position of the clip entity object at the specific path exceeds the view range limit. It might be not visible in the viewport.")
                        );
                        break;
                    }
                    case QDspx::SerializationError::SafeRangeLimitExceeded: {
                        auto e = error.staticCast<QDspx::SafeRangeLimitExceededError>();
                        addErrorItem(
                            problemModel,
                            tr("Safe range limit exceeded") + QStringLiteral(" (0x%1)").arg(QString::number(error->type(), 16), 4, '0'),
                            e->path(),
                            style()->standardIcon(QStyle::SP_MessageBoxWarning),
                            {},
                            tr("The position of the entity object at the specific path exceeds the safe project length limit (4,000,000 ticks).")
                        );
                        break;
                    }
                }
            }

            if (dspxModel.content.global.editorId != CoreInterface::dspxEditorId()) {
                addErrorItem(
                    problemModel,
                    tr("File Created With Another Application"),
                    QStringLiteral("$.content.global.editorId"),
                    style()->standardIcon(QStyle::SP_MessageBoxWarning),
                    {
                        {tr("Editor ID"), dspxModel.content.global.editorId},
                        {tr("Editor name"), dspxModel.content.global.editorName},
                    },
                    tr("This project file was created with another application. Some features may not be fully compatible or may behave differently.")
                );
            } else if (auto version = dspxModel.content.workspace.value("diffscope").value("editorVersion").toString(); !checkIsVersionCompatible(version)) {
                addErrorItem(
                    problemModel,
                    tr("File Created With Incompatible Version"),
                    QStringLiteral("$.content.workspace.diffscope.editorVersion"),
                    style()->standardIcon(QStyle::SP_MessageBoxWarning),
                    {
                        {tr("Version"), version},
                    },
                    tr("This project file was created with an newer version or test version of %1. Some features may not be fully compatible or may behave differently.").arg(QApplication::applicationDisplayName())
                );
            }

            auto customCheckResult = CoreInterface::dspxCheckerRegistry()->runCheck(dspxModel, IDspxChecker::Weak, false);
            for (const auto &warning : customCheckResult) {
                addErrorItem(
                    problemModel,
                    warning.message,
                    warning.jsonPath,
                    warning.level == IDspxChecker::Strong ? style()->standardIcon(QStyle::SP_MessageBoxWarning) : QIcon(),
                    warning.info,
                    warning.description
                );
            }

            // TODO show file structure
        }


        d->problemTreeView->resizeColumnToContents(0);
        if (problemModel->rowCount() != 0) {
            d->tabWidget->setCurrentIndex(1);
        }
    }

}
