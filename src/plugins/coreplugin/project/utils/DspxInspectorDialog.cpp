#include "DspxInspectorDialog.h"
#include "DspxInspectorDialog_p.h"

#include <sstream>

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

#include <CoreApi/runtimeinterface.h>

#include <opendspx/model.h>
#include <opendspxserializer/serializer.h>

#include <SVSCraftCore/Semver.h>
#include <SVSCraftQuick/MessageBox.h>

#include <coreplugin/CoreInterface.h>
#include <coreplugin/DspxCheckerRegistry.h>
#include <coreplugin/OpenSaveProjectFileScenario.h>

namespace Core {

    static QString getVersionStringFromModel(const opendspx::Model &model) {
        try {
            return QString::fromStdString(model.content.workspace.at("diffscope").at("editorVersion").get<std::string>());
        } catch (const nlohmann::json::exception &) {
            return {};
        }
    }

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
        pathEdit->setAccessibleName(tr("File path"));
        browseLayout->addWidget(pathEdit);
        auto browseButton = new QPushButton(tr("&Browse"));
        browseButton->setDefault(true);
        browseLayout->addWidget(browseButton);
        auto runCheckButton = new QPushButton(tr("&Run Check"));
        runCheckButton->setEnabled(false);
        layout->addWidget(runCheckButton);
        auto tabWidget = new QTabWidget;
        layout->addWidget(tabWidget);
        auto problemTreeView = new QTreeView;
        problemTreeView->setHeaderHidden(true);
        tabWidget->addTab(problemTreeView, tr("&Problems"));

        d->tabWidget = tabWidget;
        d->problemTreeView = problemTreeView;
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

    static QString joinStdStringVector(const std::vector<std::string> &vec, const QString &sep) {
        QStringList list;
        std::ranges::transform(vec, std::back_inserter(list), [](const std::string &s) {
            return QString::fromStdString(s);
        });
        return list.join(sep);
    }

    template <typename T>
    static QVariant stdAnyToVariant1(const std::any &a) {
        if (a.type() == typeid(T)) {
            if constexpr (std::is_same_v<T, std::string>) {
                return QString::fromStdString(std::any_cast<T>(a));
            } else {
                return QVariant::fromValue(std::any_cast<T>(a));
            }
        }
        return {};
    }

    template<typename... Args>
    static QVariant stdAnyToVariant(const std::any& a) {
        QVariant result;
        ((result = stdAnyToVariant1<Args>(a), !result.isNull()) || ...);
        return result;
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
        do {
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
                break;
            }
            auto data = f.readAll();

            opendspx::SerializationErrorList errors;
            std::stringstream in(data.toStdString(), std::ios::in);
            auto dspxModel = opendspx::Serializer::deserialize(in, errors, opendspx::Serializer::CheckError | opendspx::Serializer::CheckWarning);
            for (const auto &error : errors) {
                switch (error->type()) {
                    case opendspx::SerializationError::JsonParseFailure: {
                        auto e = std::static_pointer_cast<opendspx::JsonParseFailureError>(error);
                        addErrorItem(
                            problemModel,
                            tr("Fatal: Failed to parse JSON") + QStringLiteral(" (0x%1)").arg(QString::number(error->type(), 16), 4, '0'),
                            {},
                            style()->standardIcon(QStyle::SP_MessageBoxCritical),
                            {
                                {tr("Offset in file"), e->index()},
                                {tr("Error code"), e->code()},
                                {tr("Error text"), QString::fromStdString(e->message())},
                            },
                            tr("The file is not a valid JSON document.")
                        );
                        break;
                    }
                    case opendspx::SerializationError::JsonRootIsNotObject: {
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
                    case opendspx::SerializationError::UnrecognizedVersion: {
                        auto e = std::static_pointer_cast<opendspx::UnrecognizedVersionError>(error);
                        addErrorItem(
                            problemModel,
                            tr("Fatal: Unrecognized version") + QStringLiteral(" (0x%1)").arg(QString::number(error->type(), 16), 4, '0'),
                            QStringLiteral("$.version"),
                            style()->standardIcon(QStyle::SP_MessageBoxCritical),
                            {
                                {tr("Actual version"), QString::fromStdString(e->actualVersion())},
                            },
                            tr("This project file may have been created with a newer version of %1 or another application, and its version is not recognized by %1. Please try exporting the project from the application with which it was created as a version compatible with your current %1.").arg(QApplication::applicationDisplayName())
                        );
                        break;
                    }
                    case opendspx::SerializationError::InvalidDataType: {
                        auto e = std::static_pointer_cast<opendspx::InvalidDataTypeError>(error);
                        QStringList expectedTypeStrings;
                        for (auto expectedType : e->expectedTypes()) {
                            expectedTypeStrings.append(tr(jsonDataTypeNames[expectedType]));
                        }
                        addErrorItem(
                            problemModel,
                            tr("Invalid data type") + QStringLiteral(" (0x%1)").arg(QString::number(error->type(), 16), 4, '0'),
                            QString::fromStdString(e->path()),
                            style()->standardIcon(QStyle::SP_MessageBoxCritical),
                            {
                                {tr("Expected data type"), expectedTypeStrings.join(tr(", "))},
                                {tr("Actual data type"), tr(jsonDataTypeNames[e->actualType()])},
                            },
                            tr("The value at the specific path is not of the expected data type.")
                        );
                        break;
                    }
                    case opendspx::SerializationError::InvalidObjectType: {
                        auto e = std::static_pointer_cast<opendspx::InvalidObjectTypeError>(error);
                        addErrorItem(
                            problemModel,
                            tr("Invalid object type") + QStringLiteral(" (0x%1)").arg(QString::number(error->type(), 16), 4, '0'),
                            QString::fromStdString(e->path()),
                            style()->standardIcon(QStyle::SP_MessageBoxCritical),
                            {
                                {tr("Expected object type"), joinStdStringVector(e->expectedTypes(), QStringLiteral(", "))},
                                {tr("Actual object type"), QString::fromStdString(e->actualType())},
                            },
                            tr("The object at the specific path is not of the expected object type.")
                        );
                        break;
                    }
                    case opendspx::SerializationError::RangeConstraintViolation: {
                        auto e = std::static_pointer_cast<opendspx::RangeConstraintViolationError>(error);
                        addErrorItem(
                            problemModel,
                            tr("Range constraint violation") + QStringLiteral(" (0x%1)").arg(QString::number(error->type(), 16), 4, '0'),
                            QString::fromStdString(e->path()),
                            style()->standardIcon(QStyle::SP_MessageBoxCritical),
                            {
                                {tr("Expected maximum value"), !e->expectedMaximum().has_value() ? tr("None") : stdAnyToVariant<int, double>(e->expectedMaximum())},
                                {tr("Expected minimum value"), !e->expectedMinimum().has_value() ? tr("None") : stdAnyToVariant<int, double>(e->expectedMinimum())},
                                {tr("Actual value"), stdAnyToVariant<int, double>(e->actualValue())},
                            },
                            tr("The value of the property at the specific path is outside the allowed range. The value must be between the expected minimum and maximum value (inclusive).")
                        );
                        break;
                    }
                    case opendspx::SerializationError::EnumConstraintViolation: {
                        auto e = std::static_pointer_cast<opendspx::EnumConstraintViolationError>(error);
                        QList<QPair<QString, QVariant>> info;
                        for (const auto &expectedEnumValue : e->expectedEnumValues()) {
                            info.emplace_back(tr("Expected enum value"), stdAnyToVariant<int, std::string>(expectedEnumValue));
                        }
                        info.emplace_back(tr("Actual enum value"), stdAnyToVariant<int, std::string>(e->actualEnumValue()));
                        addErrorItem(
                            problemModel,
                            tr("Enum constraint violation") + QStringLiteral(" (0x%1)").arg(QString::number(error->type(), 16), 4, '0'),
                            QString::fromStdString(e->path()),
                            style()->standardIcon(QStyle::SP_MessageBoxCritical),
                            info,
                            tr("The value of the property at the specific path is not one of the allowed enum values.")
                        );
                        break;
                    }
                    case opendspx::SerializationError::MissingProperty: {
                        auto e = std::static_pointer_cast<opendspx::MissingPropertyError>(error);
                        QList<QPair<QString, QVariant>> info;
                        for (const auto &missingProperty : e->missingProperties()) {
                            info.emplace_back(tr("Missing property"), QString::fromStdString(missingProperty));
                        }
                        addErrorItem(
                            problemModel,
                            tr("Missing properties") + QStringLiteral(" (0x%1)").arg(QString::number(error->type(), 16), 4, '0'),
                            QString::fromStdString(e->path()),
                            style()->standardIcon(QStyle::SP_MessageBoxCritical),
                            info,
                            tr("One or more properties are missing in the object at the specific path.")
                        );
                        break;
                    }
                    case opendspx::SerializationError::RedundantProperty: {
                        auto e = std::static_pointer_cast<opendspx::MissingPropertyError>(error);
                        QList<QPair<QString, QVariant>> info;
                        for (const auto &redundantProperty : e->missingProperties()) {
                            info.emplace_back(tr("Redundant property"), QString::fromStdString(redundantProperty));
                        }
                        addErrorItem(
                            problemModel,
                            tr("Redundant properties") + QStringLiteral(" (0x%1)").arg(QString::number(error->type(), 16), 4, '0'),
                            QString::fromStdString(e->path()),
                            style()->standardIcon(QStyle::SP_MessageBoxCritical),
                            info,
                            tr("One or more properties are redundant in the object at the specific path.")
                        );
                        break;
                    }
                    case opendspx::SerializationError::OverlappingItem: {
                        auto e = std::static_pointer_cast<opendspx::OverlappingItemError>(error);
                        QList<QPair<QString, QVariant>> info;
                        for (auto overlappedItemIndex : e->overlappedItemIndexes()) {
                            info.emplace_back(tr("Index"), overlappedItemIndex);
                        }
                        addErrorItem(
                            problemModel,
                            tr("Overlapping items") + QStringLiteral(" (0x%1)").arg(QString::number(error->type(), 16), 4, '0'),
                            QString::fromStdString(e->path()),
                            {},
                            info,
                            tr("Items at specific indexes in the array at the specific path overlap.")
                        );
                        break;
                    }
                    case opendspx::SerializationError::ZeroLengthRange: {
                        auto e = std::static_pointer_cast<opendspx::ZeroLengthRangeError>(error);
                        addErrorItem(
                            problemModel,
                            tr("Zero-length range") + QStringLiteral(" (0x%1)").arg(QString::number(error->type(), 16), 4, '0'),
                            QString::fromStdString(e->path()),
                            {},
                            {},
                            tr("The range length of the entity object at the specific path is zero.")
                        );
                        break;
                    }
                    case opendspx::SerializationError::ErroneousClipRange: {
                        auto e = std::static_pointer_cast<opendspx::ErroneousClipRangeError>(error);
                        addErrorItem(
                            problemModel,
                            tr("Erroneous clip range") + QStringLiteral(" (0x%1)").arg(QString::number(error->type(), 16), 4, '0'),
                            QString::fromStdString(e->path()),
                            {},
                            {},
                            tr("The clipping range of the clip entity object at the specific path exceeds its range limit.")
                        );
                        break;
                    }
                    case opendspx::SerializationError::ErroneousClipPosition: {
                        auto e = std::static_pointer_cast<opendspx::ErroneousClipPositionError>(error);
                        addErrorItem(
                            problemModel,
                            tr("Erroneous clip position") + QStringLiteral(" (0x%1)").arg(QString::number(error->type(), 16), 4, '0'),
                            QString::fromStdString(e->path()),
                            {},
                            {},
                            tr("The position of the clip entity object at the specific path exceeds the view range limit. It might be not visible in the viewport.")
                        );
                        break;
                    }
                    case opendspx::SerializationError::SafeRangeLimitExceeded: {
                        auto e = std::static_pointer_cast<opendspx::SafeRangeLimitExceededError>(error);
                        addErrorItem(
                            problemModel,
                            tr("Safe range limit exceeded") + QStringLiteral(" (0x%1)").arg(QString::number(error->type(), 16), 4, '0'),
                            QString::fromStdString(e->path()),
                            style()->standardIcon(QStyle::SP_MessageBoxWarning),
                            {},
                            tr("The position of the entity object at the specific path exceeds the safe project length limit (4,000,000 ticks).")
                        );
                        break;
                    }
                }
            }
            if (errors.containsFatal())
                break;
            if (dspxModel.content.global.editorId != CoreInterface::dspxEditorId()) {
                addErrorItem(
                    problemModel,
                    tr("File Created With Another Application"),
                    QStringLiteral("$.content.global.editorId"),
                    style()->standardIcon(QStyle::SP_MessageBoxWarning),
                    {
                        {tr("Editor ID"), QString::fromStdString(dspxModel.content.global.editorId)},
                        {tr("Editor name"), QString::fromStdString(dspxModel.content.global.editorName)},
                    },
                    tr("This project file was created with another application. Some features may not be fully compatible or may behave differently.")
                );
            } else if (auto version = getVersionStringFromModel(dspxModel); !checkIsVersionCompatible(version)) {
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
        } while (false);

        d->problemTreeView->expandAll();
        d->problemTreeView->resizeColumnToContents(0);
        if (problemModel->rowCount() == 0) {
            SVS::MessageBox::success(RuntimeInterface::qmlEngine(), windowHandle(), tr("No problems found"), tr("The project file is valid and no problems were found."));
        }
    }

}
